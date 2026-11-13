// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QColor>
#include <QFont>
#include <QList>
#include <QSize>
#include <QString>

#include <wayland-client.h>

QT_BEGIN_NAMESPACE
class QImage;
QT_END_NAMESPACE

namespace QtWaylandClient
{
class QWaylandDisplay;
}

namespace dock
{

// A single role-less wl_surface owned by the dock, one per tooltip, repainted with
// its tooltip text on every request. treeland draws the popover pill, the blur and
// the shadow behind it, so the surface stays fully transparent except for the
// text and carries its own inner padding. Giving each tooltip its own surface is
// what lets the compositor animate the switch between two tooltips: repainting a
// single surface in place only changes the content.
class TreelandTooltipSurface
{
public:
    explicit TreelandTooltipSurface(QtWaylandClient::QWaylandDisplay *display);
    ~TreelandTooltipSurface();

    TreelandTooltipSurface(const TreelandTooltipSurface &) = delete;
    TreelandTooltipSurface &operator=(const TreelandTooltipSurface &) = delete;

    // Repaints and commits the surface; returns the new logical size, or an
    // empty size when the surface could not be prepared.
    QSize setText(const QString &text, qreal devicePixelRatio);

    // Draws the tooltip content: transparent background, centred text. Kept
    // separate from the Wayland plumbing so it can be checked on its own.
    static QImage renderImage(const QString &text, const QFont &font, const QColor &textColor, int scale);

    struct wl_surface *surface() const
    {
        return m_surface;
    }
    bool isValid() const
    {
        return m_surface != nullptr;
    }

private:
    struct Buffer {
        TreelandTooltipSurface *owner = nullptr;
        struct wl_buffer *buffer = nullptr;
        uchar *mapping = nullptr;
        size_t size = 0;
        struct wl_buffer_listener releaseListener{};
    };

    bool ensureSurface();
    Buffer *createBuffer(const QImage &image);
    void destroyBuffer(Buffer *buffer);

    static void bufferReleased(void *data, struct wl_buffer *buffer);

    QtWaylandClient::QWaylandDisplay *m_display = nullptr;
    struct wl_surface *m_surface = nullptr;
    // Buffers handed to the compositor and not released yet. Each one is freed
    // by the release event.
    QList<Buffer *> m_buffers;
    int m_bufferScale = 0;
    // The same text is requested again whenever the pointer moves between
    // items with the same title; skip repainting then. The text style is part
    // of the key because a theme or font change has to repaint as well.
    QString m_lastText;
    QFont m_lastFont;
    QColor m_lastColor;
    QSize m_lastSize;
};

}
