// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "treelandtooltipsurface.h"

#include <DFontManager>
#include <DGuiApplicationHelper>

#include <QColor>
#include <QFont>
#include <QFontMetricsF>
#include <QImage>
#include <QPainter>
#include <QPalette>
#include <QtMath>

#include <private/qwaylanddisplay_p.h>

#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

namespace dock
{

namespace
{
// treeland adds no padding of its own: the popover frame is exactly the
// surface, so the text has to carry the padding DTK's tooltip used to have.
constexpr int HorizontalPadding = 8;
constexpr int VerticalPadding = 4;
// Upper bound so a pathological app title cannot create a huge buffer.
constexpr int MaxTextWidth = 1024;

// The fallback PanelToolTip draws its text with DTK's t8 font, capitalised, so
// the compositor-drawn tooltip has to use the very same font (the QML
// DTK.fontManager singleton is this DFontManager) or the two paths would render
// visibly different tooltips.
QFont tooltipFont()
{
    const Dtk::Gui::DFontManager fontManager;
    QFont font = fontManager.t8();
    font.setCapitalization(QFont::Capitalize);
    return font;
}

// PanelToolTip takes palette.brightText: DTK's tooltip is theme-coloured and
// opaque, while ToolTipText is only 85% opaque and goes see-through over the
// tint treeland paints behind the surface.
QColor tooltipTextColor()
{
    return Dtk::Gui::DGuiApplicationHelper::instance()->applicationPalette().color(QPalette::BrightText);
}
}

TreelandTooltipSurface::TreelandTooltipSurface(QtWaylandClient::QWaylandDisplay *display)
    : m_display(display)
{
}

TreelandTooltipSurface::~TreelandTooltipSurface()
{
    while (!m_buffers.isEmpty())
        destroyBuffer(m_buffers.takeLast());

    if (m_surface) {
        wl_surface_destroy(m_surface);
        m_surface = nullptr;
    }
}

bool TreelandTooltipSurface::ensureSurface()
{
    if (m_surface)
        return true;
    if (!m_display || !m_display->compositor() || !m_display->shm())
        return false;

    m_surface = wl_compositor_create_surface(m_display->compositor()->object());
    return m_surface != nullptr;
}

QSize TreelandTooltipSurface::setText(const QString &text, qreal devicePixelRatio)
{
    if (!ensureSurface())
        return {};

    const QFont font = tooltipFont();
    const QColor textColor = tooltipTextColor();
    // Ceil, like Qt's own wayland windows: wl_surface buffer_scale is an integer
    // and treeland never scales the tooltip itself, so rounding 1.25 down to 1
    // would hand the compositor a 1x buffer to draw larger, i.e. blurry text
    // next to the crisp 2x dock.
    const int scale = qMax(1, qCeil(devicePixelRatio > 0 ? devicePixelRatio : 1.0));
    if (text == m_lastText && font == m_lastFont && textColor == m_lastColor && scale == m_bufferScale && !m_lastSize.isEmpty())
        return m_lastSize;

    const QImage image = renderImage(text, font, textColor, scale);
    if (image.isNull())
        return {};

    auto *buffer = createBuffer(image);
    if (!buffer)
        return {};

    if (scale != m_bufferScale) {
        wl_surface_set_buffer_scale(m_surface, scale);
        m_bufferScale = scale;
    }

    const QSize logicalSize = image.deviceIndependentSize().toSize();
    wl_surface_attach(m_surface, buffer->buffer, 0, 0);
    wl_surface_damage(m_surface, 0, 0, logicalSize.width(), logicalSize.height());
    wl_surface_commit(m_surface);
    // The caller sends the protocol request right after this, so push both out
    // together instead of waiting for Qt's next event loop round.
    if (auto *display = m_display->wl_display())
        wl_display_flush(display);

    m_lastText = text;
    m_lastFont = font;
    m_lastColor = textColor;
    m_lastSize = logicalSize;
    return logicalSize;
}

QImage TreelandTooltipSurface::renderImage(const QString &text, const QFont &font, const QColor &textColor, int scale)
{
    QFontMetricsF metrics(font);
    const QString shownText = metrics.horizontalAdvance(text) > MaxTextWidth ? metrics.elidedText(text, Qt::ElideRight, MaxTextWidth) : text;
    const int textWidth = qMin(qCeil(metrics.horizontalAdvance(shownText)), MaxTextWidth);
    const QSize logicalSize(textWidth + 2 * HorizontalPadding, qCeil(metrics.height()) + 2 * VerticalPadding);

    QImage image(logicalSize * scale, QImage::Format_ARGB32_Premultiplied);
    if (image.isNull())
        return {};
    image.setDevicePixelRatio(scale);
    image.fill(Qt::transparent);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::TextAntialiasing);
    painter.setFont(font);
    painter.setPen(textColor);
    painter.drawText(QRectF(QPointF(0, 0), QSizeF(logicalSize)), Qt::AlignCenter, shownText);
    painter.end();

    return image;
}

TreelandTooltipSurface::Buffer *TreelandTooltipSurface::createBuffer(const QImage &image)
{
    const QSize deviceSize = image.size();
    if (deviceSize.isEmpty())
        return nullptr;

    const int stride = deviceSize.width() * 4;
    const size_t size = static_cast<size_t>(stride) * deviceSize.height();

    char name[64];
    snprintf(name, sizeof(name), "/dde-shell-tooltip-%d", static_cast<int>(::getpid()));
    const int fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
    if (fd < 0)
        return nullptr;
    shm_unlink(name);

    if (ftruncate(fd, static_cast<off_t>(size)) < 0) {
        ::close(fd);
        return nullptr;
    }

    void *mapping = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mapping == MAP_FAILED) {
        ::close(fd);
        return nullptr;
    }

    for (int y = 0; y < deviceSize.height(); ++y) {
        memcpy(static_cast<uchar *>(mapping) + stride * y, image.constScanLine(y), static_cast<size_t>(stride));
    }

    struct wl_shm_pool *pool = wl_shm_create_pool(m_display->shm()->object(), fd, static_cast<int>(size));
    ::close(fd);
    if (!pool) {
        munmap(mapping, size);
        return nullptr;
    }

    struct wl_buffer *wlBuffer = wl_shm_pool_create_buffer(pool, 0, deviceSize.width(), deviceSize.height(), stride, WL_SHM_FORMAT_ARGB8888);
    wl_shm_pool_destroy(pool);
    if (!wlBuffer) {
        munmap(mapping, size);
        return nullptr;
    }

    auto *buffer = new Buffer;
    buffer->owner = this;
    buffer->buffer = wlBuffer;
    buffer->mapping = static_cast<uchar *>(mapping);
    buffer->size = size;
    buffer->releaseListener.release = &TreelandTooltipSurface::bufferReleased;
    wl_buffer_add_listener(wlBuffer, &buffer->releaseListener, buffer);

    m_buffers.append(buffer);
    return buffer;
}

void TreelandTooltipSurface::destroyBuffer(Buffer *buffer)
{
    if (!buffer)
        return;
    if (buffer->buffer)
        wl_buffer_destroy(buffer->buffer);
    if (buffer->mapping)
        munmap(buffer->mapping, buffer->size);
    delete buffer;
}

void TreelandTooltipSurface::bufferReleased(void *data, [[maybe_unused]] struct wl_buffer *wlBuffer)
{
    auto *buffer = static_cast<Buffer *>(data);
    if (!buffer || !buffer->owner)
        return;

    // The compositor keeps whatever it still needs from the released buffer,
    // so the client is free to drop it right away.
    buffer->owner->m_buffers.removeOne(buffer);
    buffer->owner->destroyBuffer(buffer);
}

}
