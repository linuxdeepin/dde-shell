// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "appitem.h"
#include "x11preview.h"
#include "abstractwindow.h"
#include "x11windowmonitor.h"

#include <dguiapplicationhelper.h>
#include <qt5/QtCore/qnamespace.h>
#include <unistd.h>

#include <QFile>
#include <QTimer>
#include <QEvent>
#include <QWindow>
#include <QPixmap>
#include <QLayout>
#include <QPainter>
#include <QBoxLayout>
#include <QByteArray>
#include <QDBusReply>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDBusInterface>
#include <QLoggingCategory>
#include <QDBusUnixFileDescriptor>

#include <DStyle>

#include <DWindowManagerHelper>
#include <DGuiApplicationHelper>

Q_LOGGING_CATEGORY(x11WindowPreview, "dde.shell.dock.taskmanager.x11WindowPreview")

#define PREVIEW_TITLE_HEGIHT 20
#define PREVIEW_CONTENT_HEIGHT 118
#define PREVIEW_CONTENT_WIDTH 208
#define PREVIEW_HOVER_BORDER 4
#define WM_HELPER DWindowManagerHelper::instance()

DGUI_USE_NAMESPACE
DS_BEGIN_NAMESPACE

namespace dock {

X11WindowPreviewContent::X11WindowPreviewContent(const QPointer<AbstractWindow> &window, QWidget* parent)
    : QWidget(parent)
    , m_isHovered(false)
    , m_isMinimized(false)
{
    m_timer = new QTimer;
    m_timer->setInterval(500);

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->addStretch();
    layout->setAlignment(Qt::AlignTop);
    layout->setContentsMargins(WM_HELPER->hasComposite() ? QMargins(5, 5, 5, 5) : QMargins(0, 0, 0, 0));

    m_previewLabel = new QLabel(this);
    m_closeButton = new DIconButton;
    m_closeButton->setFixedSize(PREVIEW_TITLE_HEGIHT, PREVIEW_TITLE_HEGIHT);
    m_closeButton->setVisible(false);
    m_closeButton->setIcon(QIcon::fromTheme("close"));
    m_closeButton->setFlat(true);
    layout->addWidget(m_closeButton);

    setMouseTracking(true);
    setPrewviewContentWindow(window);

    connect(m_closeButton, &DIconButton::clicked, this, [this](){
        if (m_window.isNull()) return;
        m_window->close();
    });

    connect(m_timer, &QTimer::timeout, this, [this](){
        fetchWindowPreview();
        update();
    });

    connect(WM_HELPER, &DWindowManagerHelper::hasCompositeChanged, this, [this, layout](){
        if (WM_HELPER->hasComposite()) {
            setFixedSize(PREVIEW_CONTENT_WIDTH, PREVIEW_CONTENT_HEIGHT);
            layout->setContentsMargins(5, 5, 5, 5);
        } else {
            m_timer->stop();
            setFixedSize(PREVIEW_CONTENT_WIDTH, PREVIEW_TITLE_HEGIHT);
            layout->setContentsMargins(0, 0, 0, 0);
        }
    });

    if (WM_HELPER->hasComposite()) {
        setFixedSize(PREVIEW_CONTENT_WIDTH, PREVIEW_CONTENT_HEIGHT);
    } else {
        setFixedSize(PREVIEW_CONTENT_WIDTH, PREVIEW_TITLE_HEGIHT);
    }
}

void X11WindowPreviewContent::setPrewviewContentWindow(const QPointer<AbstractWindow> &window)
{
    if (m_window == window) return;

    if (!m_window.isNull()) {
        m_window->disconnect(this);
    }

    m_window = window;
    connect(window.get(), &QObject::destroyed, this, [this](){
        Q_EMIT windowDestoried(this);
    });

    if (WM_HELPER->hasComposite()) {
        QMetaObject::invokeMethod(this,&X11WindowPreviewContent::fetchWindowPreview);
    }

    repaint();
}

void X11WindowPreviewContent::fetchWindowPreview()
{
    // TODO: check kwin is load screenshot plugin
    if (m_window.isNull() || !WM_HELPER->hasComposite()) return;

    // pipe read write fd
    int fd[2];

    if (pipe(fd) < 0) {
        qDebug() << "failed to create pipe";
        return;
    }

    QDBusInterface interface(QStringLiteral("org.kde.KWin"), QStringLiteral("/org/kde/KWin/ScreenShot2"), QStringLiteral("org.kde.KWin.ScreenShot2"));
    // 第一个参数，winID或者UUID
    QList<QVariant> args;
    args << QVariant::fromValue(QString::number(m_window->id()));
    // 第二个参数，需要截图的选项
    QVariantMap option;
    option["include-decoration"] = true;
    option["include-cursor"] = false;
    option["native-resolution"] = true;
    args << QVariant::fromValue(option);
    // 第三个参数，文件描述符
    args << QVariant::fromValue(QDBusUnixFileDescriptor(fd[1]));

    QDBusReply<QVariantMap> reply = interface.callWithArgumentList(QDBus::Block, QStringLiteral("CaptureWindow"), args);
    if(!reply.isValid()) {
        ::close(fd[1]);
        ::close(fd[0]);
        qDebug() << "get current workspace background error: "<< reply.error().message();
        return;
    }

    // close write
    ::close(fd[1]);

    QVariantMap imageInfo = reply.value();
    int imageWidth = imageInfo.value("width").toUInt();
    int imageHeight = imageInfo.value("height").toUInt();
    int imageStride = imageInfo.value("stride").toUInt();
    int imageFormat = imageInfo.value("format").toUInt();

    QFile file;
    if (!file.open(fd[0], QIODevice::ReadOnly)) {
        file.close();
        ::close(fd[0]);
        return;
    }

    QImage::Format qimageFormat = static_cast<QImage::Format>(imageFormat);
    int bitsCountPerPixel = QImage::toPixelFormat(qimageFormat).bitsPerPixel();

    QByteArray fileContent = file.read(imageHeight * imageWidth * bitsCountPerPixel / 8);
    QImage image(reinterpret_cast<uchar *>(fileContent.data()), imageWidth, imageHeight, imageStride, qimageFormat);
    m_pixmap = QPixmap::fromImage(image);

    // close read
    ::close(fd[0]);
}

void X11WindowPreviewContent::mouseReleaseEvent(QMouseEvent* event)
{
    if (m_window.isNull()) return;

    m_isMinimized = false;
    m_window->activate();
}

void X11WindowPreviewContent::enterEvent(QEnterEvent*)
{
    m_closeButton->setVisible(true);
    m_isHovered = true;
    m_isMinimized = m_window.isNull() ? false : m_window->isMinimized();

    update();
    Q_EMIT entered(m_window);
}

void X11WindowPreviewContent::leaveEvent(QEvent*)
{
    m_closeButton->setVisible(false);
    m_isHovered = false;
    if (m_isMinimized && !m_window.isNull()) m_window->minimize();

    update();
    Q_EMIT exited();
}

void X11WindowPreviewContent::paintEvent(QPaintEvent *event)
{
    const auto ratio = devicePixelRatioF();

    QWidget::paintEvent(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    DStyleHelper dstyle(style());
    const int radius = dstyle.pixelMetric(DStyle::PM_FrameRadius);

    if (WM_HELPER->hasComposite()) {
        auto size = m_pixmap.size().scaled(rect().size(), Qt::KeepAspectRatio);
        QRect imageRect((width() - size.width()) / 2, (height() - size.height()) / 2, size.width(), size.height());
        painter.drawPixmap(imageRect, m_pixmap, m_pixmap.rect());
    } else {
        painter.save();
        QPen pen;
        pen.setColor(Qt::black);
        painter.setPen(pen);
        painter.fillRect(rect(), QColor(255, 255, 255, 255 * .2));
        painter.drawText(rect(), QFontMetrics(font()).elidedText(m_window->title(), Qt::TextElideMode::ElideRight, width()-  m_closeButton->width()));
        painter.restore();
    }
}

X11WindowPreviewContainer::X11WindowPreviewContainer(X11WindowMonitor* monitor, QWidget *parent)
    : DBlurEffectWidget(parent)
    , m_contentLayout(nullptr)
    , m_direction(0)
    , m_x11Monitor(monitor)
    , m_isDockPreviewCount(0)
{
    m_hideTimer = new QTimer(this);
    m_hideTimer->setSingleShot(true);
    m_hideTimer->setInterval(300);

    m_exitTimer = new QTimer(this);
    m_exitTimer->setSingleShot(true);
    m_exitTimer->setInterval(100);

    setWindowFlags(Qt::ToolTip | Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus | Qt::FramelessWindowHint);
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    initUI();

    connect(m_hideTimer, &QTimer::timeout, this, &X11WindowPreviewContainer::callHide);
    connect(m_exitTimer, &QTimer::timeout, this, [this](){
        if (m_previewItem.isNull()) return;
        m_closeAllButton->setVisible(true);
        m_currentWindowIcon->setPixmap(QIcon::fromTheme(m_previewItem->icon()).pixmap(PREVIEW_TITLE_HEGIHT, PREVIEW_TITLE_HEGIHT));
        m_currentWindowTitleStr = m_previewItem->name();

        m_currentWindowTitle->setText(
            QFontMetrics(m_currentWindowTitle->font()).elidedText(m_currentWindowTitleStr, Qt::TextElideMode::ElideRight, width() - m_currentWindowIcon->width() - m_closeAllButton->width() - 25)
        );

        if (WM_HELPER->hasComposite()) {
            m_x11Monitor->cancelPreviewWindow();
        }
    });

    connect(m_closeAllButton, &DIconButton::clicked, this, [this](){
        if (m_previewItem.isNull()) return;
        for (auto window : m_previewItem->getAppendWindows()) {
            window->close();
        }
    });

}

void X11WindowPreviewContainer::showPreview(const QPointer<AppItem> &item, const QPointer<QWindow> &window, int32_t previewXoffset, int32_t previewYoffset, uint32_t direction)
{
    if (!m_previewItem.isNull()) {
        m_previewItem->disconnect(this);
    }

    m_previewItem = item;
    m_baseWindow = window;
    m_previewXoffset = previewXoffset;
    m_previewYoffset = previewYoffset;
    m_direction = direction;

    m_isDockPreviewCount += 1;

    m_currentWindowIcon->setPixmap(QIcon::fromTheme(item->icon()).pixmap(PREVIEW_TITLE_HEGIHT, PREVIEW_TITLE_HEGIHT));

    m_currentWindowTitleStr = item->name();
    updateCurrentHoveredTitle();

    auto needPreviewWindows = item->getAppendWindows();
    auto currentAddedCount = m_contentLayout->count();
    auto minSize = std::min(int(needPreviewWindows.count()), currentAddedCount);

    for (int i = 0; i < minSize; i++) {
        auto item = m_contentLayout->itemAt(i);
        X11WindowPreviewContent* previewContent = reinterpret_cast<X11WindowPreviewContent*>(item->widget());
        previewContent->setPrewviewContentWindow(needPreviewWindows[i]);
    }

    for (int i = minSize; i < needPreviewWindows.count(); i++) {
        addNeedPreviewWindow(needPreviewWindows[i]);
    }

    clearSpecifiedCountPreviews(currentAddedCount - minSize);
    connect(item, &AppItem::appendedWindow, this, &X11WindowPreviewContainer::addNeedPreviewWindow, Qt::UniqueConnection);

    if (isHidden()) {
        show();
    }
}

void X11WindowPreviewContainer::addNeedPreviewWindow(const QPointer<AbstractWindow> &window)
{
    auto content = new X11WindowPreviewContent(window);
    m_contentLayout->addWidget(content);

    connect(WM_HELPER, &DWindowManagerHelper::hasCompositeChanged, this, &X11WindowPreviewContainer::updateLayout);
    connect(content, &X11WindowPreviewContent::entered, m_exitTimer, &QTimer::stop);
    connect(content, &X11WindowPreviewContent::entered, this, &X11WindowPreviewContainer::updateHoveredWindow);

    connect(content, &X11WindowPreviewContent::exited, this, [this](){
        m_hoverRect = QRect();
        m_hoveredWindow->disconnect(this);
        m_hoveredWindow = nullptr;
        m_exitTimer->start();
        repaint();
    });

    connect(content, &X11WindowPreviewContent::windowDestoried, this, [this](X11WindowPreviewContent* window){
        m_contentLayout->removeWidget(window);

        if (m_hoverRect.contains(window->geometry())) {
            m_hoverRect = QRect();
        }

        window->deleteLater();
        if (m_contentLayout->count() == 0) hide();
    });
}

void X11WindowPreviewContainer::updateLayout()
{
    m_contentLayout->setDirection(m_direction % 2 == 0 && WM_HELPER->hasComposite() ? QBoxLayout::LeftToRight : QBoxLayout::TopToBottom);
    m_contentLayout->setSpacing(WM_HELPER->hasComposite() ? 10 : 5);
}

void X11WindowPreviewContainer::callHide()
{
    if (m_isPreviewEntered) return;
    if (m_isDockPreviewCount > 0) return;

    hide();
}

void X11WindowPreviewContainer::hidePreView()
{
    m_isDockPreviewCount -= 1;
    m_isDockPreviewCount = std::max(0, m_isDockPreviewCount);
    m_hideTimer->start();
}

void X11WindowPreviewContainer::enterEvent(QEnterEvent* event)
{
    m_isPreviewEntered = true;
    return DBlurEffectWidget::enterEvent(event);
}

void X11WindowPreviewContainer::leaveEvent(QEvent* event)
{
    m_isPreviewEntered = false;
    m_hideTimer->start();
    return DBlurEffectWidget::leaveEvent(event);
}

void X11WindowPreviewContainer::showEvent(QShowEvent *event)
{
    updateLayout();
    return DBlurEffectWidget::showEvent(event);
}

void X11WindowPreviewContainer::hideEvent(QHideEvent*)
{
    m_previewItem = QPointer<AppItem>(nullptr);
    m_hoverRect = QRect();
    clearSpecifiedCountPreviews(m_contentLayout->count());
}

void X11WindowPreviewContainer::resizeEvent(QResizeEvent *event)
{
    updateCurrentHoveredTitle();
    updatePosition();
}

void X11WindowPreviewContainer::paintEvent(QPaintEvent* event)
{
    DBlurEffectWidget::paintEvent(event);

    if (!m_hoverRect.isValid()) return;

    const auto ratio = devicePixelRatioF();

    QWidget::paintEvent(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    DStyleHelper dstyle(style());
    const int radius = dstyle.pixelMetric(DStyle::PM_FrameRadius);

    QPen pen;
    pen.setWidth(PREVIEW_HOVER_BORDER);
    pen.setColor(QColor(0, 0, 0, 255 * 0.3));
    painter.setPen(pen);
    if (WM_HELPER->hasComposite()) {
        painter.drawRoundedRect(m_hoverRect, radius * ratio, radius * ratio);
    } else {
        painter.drawRect(m_hoverRect);
    }
}

void X11WindowPreviewContainer::clearSpecifiedCountPreviews(int count)
{
    count = std::min(m_contentLayout->count(), count);
    QLayoutItem *item;
    while (count >= 0) {
        item = m_contentLayout->takeAt(--count);
        if (item == nullptr) break;
        delete item->widget();
        delete item;
    }
}

void X11WindowPreviewContainer::updatePosition()
{
    auto dockWindowPosition = m_baseWindow->position();
    int xPosition = dockWindowPosition.x() + m_previewXoffset;
    int yPosition = dockWindowPosition.y() + m_previewYoffset;
    switch(m_direction) {
        case 0: {
            xPosition -= width() / 2;
            break;
        }
        case 1: {
            xPosition -= width();
            yPosition -= height() / 2;
            break;
        }
        case 2: {
            xPosition -= width() / 2;
            yPosition -= height();
            break;
        }
        case 3: {
            yPosition -= height() / 2;
            break;
        }
        default: {
            qCWarning(x11WindowPreview) << QStringLiteral("unknown dock direction");
            break;
        }
    }

    move(xPosition, yPosition);
}

void X11WindowPreviewContainer::updateCurrentHoveredTitle()
{
    m_currentWindowTitle->setText(
        QFontMetrics(m_currentWindowTitle->font()).elidedText(m_currentWindowTitleStr, Qt::TextElideMode::ElideRight, width() - m_currentWindowIcon->width() - m_closeAllButton->width() - 25)
    );
}

void X11WindowPreviewContainer::initUI()
{
    m_mainLayout = new QVBoxLayout;
    QHBoxLayout* titleLayout = new QHBoxLayout;
    m_contentLayout = new QBoxLayout(QBoxLayout::LeftToRight);
    setLayout(m_mainLayout);

    titleLayout->setContentsMargins(5, 0, 5, 0);

    m_currentWindowIcon = new QLabel;
    m_currentWindowTitle = new QLabel;
    m_currentWindowIcon->setFixedSize(PREVIEW_TITLE_HEGIHT, PREVIEW_TITLE_HEGIHT);

    m_closeAllButton = new DIconButton;
    m_closeAllButton->setFixedSize(PREVIEW_TITLE_HEGIHT, PREVIEW_TITLE_HEGIHT);
    m_closeAllButton->setIcon(QIcon::fromTheme("close"));
    m_closeAllButton->setFlat(true);

    m_currentWindowIcon->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_currentWindowTitle->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    auto updateWindowTitleColorType = [this](){
        QPalette pa = palette();
        auto type = DGuiApplicationHelper::instance()->themeType();
        pa.setColor(QPalette::WindowText, type == DGuiApplicationHelper::ColorType::LightType ? Qt::black : Qt::white);
        m_currentWindowTitle->setPalette(pa);
    };

    updateWindowTitleColorType();

    connect(DGuiApplicationHelper::instance(), & DGuiApplicationHelper::themeTypeChanged, this , updateWindowTitleColorType);

    titleLayout->addWidget(m_currentWindowIcon);
    titleLayout->addWidget(m_currentWindowTitle);
    titleLayout->addStretch();
    titleLayout->addWidget(m_closeAllButton);

    m_mainLayout->addLayout(titleLayout);
    m_mainLayout->addLayout(m_contentLayout);

    m_contentLayout->setContentsMargins(5, 5, 5, 5);
    m_mainLayout->setSizeConstraint(QLayout::SetFixedSize);
}

void X11WindowPreviewContainer::updateHoveredWindow(AbstractWindow *window)
{
    // 1. paint hovered background
    auto item = qobject_cast<X11WindowPreviewContent*>(sender());
    m_hoverRect = item->geometry().marginsAdded(QMargins(PREVIEW_HOVER_BORDER, PREVIEW_HOVER_BORDER, PREVIEW_HOVER_BORDER, PREVIEW_HOVER_BORDER));

    // 2. make close all button insivible
    m_closeAllButton->setVisible(false);

    // 3. update pixmap icon
    if (window == nullptr) return;
    QPixmap pix;
    const QStringList strs = window->icon().split("base64,");

    if (strs.size() == 2) {
        pix.loadFromData(QByteArray::fromBase64(strs.at(1).toLatin1()));
        pix = pix.scaled(PREVIEW_TITLE_HEGIHT, PREVIEW_TITLE_HEGIHT);
    }

    if (!pix.isNull()) {
        m_currentWindowIcon->setPixmap(pix);
    }

    // 4. update title
    m_hoveredWindow = window;
    connect(window, &AbstractWindow::titleChanged, this, [this, window](){
        m_currentWindowTitleStr = window->title();
        updateCurrentHoveredTitle();
    });

    m_currentWindowTitleStr = window->title();
    updateCurrentHoveredTitle();

    // 5. enter window preview
    if (WM_HELPER->hasComposite()) {
        m_x11Monitor->previewWindow(window->id());
    }

    repaint();
}
}

DS_END_NAMESPACE
