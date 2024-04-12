// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "appitem.h"
#include "x11utils.h"
#include "x11preview.h"
#include "abstractwindow.h"
#include "x11windowmonitor.h"

#include <cstdint>
#include <unistd.h>

#include <QFile>
#include <QTimer>
#include <QEvent>
#include <QWindow>
#include <QPixmap>
#include <QLayout>
#include <QScreen>
#include <QPainter>
#include <QByteArray>
#include <QDBusReply>
#include <QPushButton>
#include <QMouseEvent>
#include <QDBusInterface>
#include <QLoggingCategory>
#include <QDBusUnixFileDescriptor>

#include <DStyle>
#include <DListView>

#include <DWindowManagerHelper>
#include <DGuiApplicationHelper>

Q_LOGGING_CATEGORY(x11WindowPreview, "dde.shell.dock.taskmanager.x11WindowPreview")

#define PREVIEW_TITLE_HEGIHT 20
#define PREVIEW_CONTENT_HEIGHT 122
#define PREVIEW_CONTENT_WIDTH 212
#define PREVIEW_HOVER_BORDER 4
#define WM_HELPER DWindowManagerHelper::instance()

DGUI_USE_NAMESPACE

namespace dock {
enum {
    WindowIdRole = Qt::UserRole + 1,
    WindowTitleRole,
    WindowIconRole,
};

class AppItemWindowModel : public QAbstractListModel
{
public:
    AppItemWindowModel(QObject* parent = nullptr) : QAbstractListModel(parent) {}

    int rowCount(const QModelIndex& parent = QModelIndex()) const override
    {
        return m_item->getAppendWindows().size();
    }

    QVariant data(const QModelIndex& index, int role = WindowIdRole) const override
    {
        if (!index.isValid() || index.row() >= rowCount())
            return QVariant();
        
        switch (role) {
            case WindowIdRole: {
                return m_item->getAppendWindows()[index.row()]->id();
            }
            case WindowTitleRole: {
                return m_item->getAppendWindows()[index.row()]->title();
            }
            case WindowIconRole: {
                return m_item->getAppendWindows()[index.row()]->icon();
            }
        }

        return QVariant();
    }

    void setData(const QPointer<AppItem>& item)
    {
        if (!m_item.isNull()) {
            m_item->disconnect(this);
        }

        beginResetModel();
        m_item = item;
        endResetModel();

        connect(item, &AppItem::dataChanged, this, [this](){
            beginResetModel();
            endResetModel();
        });
    }

private:
    QPointer<AppItem> m_item;
};

class AppItemWindowDeletegate : public QAbstractItemDelegate
{
private:
    QListView *m_listView;
    X11WindowPreviewContainer* m_parent;

public:
    AppItemWindowDeletegate(QListView *listview, X11WindowPreviewContainer *parent = nullptr) : QAbstractItemDelegate(parent)
    {
        m_listView = listview;
        m_parent = parent;
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        if (WM_HELPER->hasComposite()) {
            auto pixmap = fetchWindowPreview(index.data(WindowIdRole).toUInt()); 
            auto size = pixmap.size().scaled(option.rect.size() - QSize(8, 8), Qt::KeepAspectRatio);
            QRect imageRect((option.rect.left() + (option.rect.width() - size.width()) / 2), (option.rect.top() + (option.rect.height() - size.height()) / 2), size.width(), size.height());
            painter->drawPixmap(imageRect, pixmap);
        } else {
            auto rect = option.rect.marginsAdded(QMargins(-4,-4, 4, 4));
            auto text = QFontMetrics(m_parent->font()).elidedText(index.data(WindowTitleRole).toString(), Qt::TextElideMode::ElideRight, rect.width() - PREVIEW_TITLE_HEGIHT);
            painter->drawText(rect, text);
        }

        if (!option.state.testFlag(QStyle::State_MouseOver)) {
            m_listView->closePersistentEditor(index);
            return;
        }
        m_listView->openPersistentEditor(index);

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        DStyleHelper dstyle(m_listView->style());
        const int radius = dstyle.pixelMetric(DStyle::PM_FrameRadius);

        QPen pen;
        pen.setWidth(PREVIEW_HOVER_BORDER);
        pen.setColor(QColor(0, 0, 0, 255 * 0.3));
        painter->setPen(pen);
        auto hoverRect = option.rect.marginsAdded(QMargins(-2,-2,-2,-2));
        if (WM_HELPER->hasComposite()) {
            painter->drawRoundedRect(hoverRect, radius, radius);
        } else {
            painter->drawRect(hoverRect);
        }
        painter->restore();
    }

    virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        return QSize(PREVIEW_CONTENT_WIDTH, WM_HELPER->hasComposite() ? PREVIEW_CONTENT_HEIGHT : (PREVIEW_TITLE_HEGIHT + 8));
    }

    virtual QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        auto closeButton = new DIconButton(parent);
        closeButton->setFixedSize(PREVIEW_TITLE_HEGIHT, PREVIEW_TITLE_HEGIHT);
        closeButton->move(option.rect.topRight() - QPoint(PREVIEW_TITLE_HEGIHT + 4, -4));
        closeButton->setVisible(true);
        closeButton->setIcon(QIcon::fromTheme("close"));
        closeButton->setFlat(true);

        connect(closeButton, &DIconButton::clicked, this, [this, index](){
            X11Utils::instance()->closeWindow(index.data(WindowIdRole).toInt());
            if (m_listView->model()->rowCount() == 1) {
                m_parent->hide();
            }
        });

        return closeButton;
    }

private:
    QPixmap fetchWindowPreview(const uint32_t& winId) const
    {
        // TODO: check kwin is load screenshot plugin
        if (!WM_HELPER->hasComposite()) return QPixmap();

        // pipe read write fd
        int fd[2];

        if (pipe(fd) < 0) {
            qDebug() << "failed to create pipe";
            return QPixmap();
        }

        QDBusInterface interface(QStringLiteral("org.kde.KWin"), QStringLiteral("/org/kde/KWin/ScreenShot2"), QStringLiteral("org.kde.KWin.ScreenShot2"));
        // 第一个参数，winID或者UUID
        QList<QVariant> args;
        args << QVariant::fromValue(QString::number(winId));
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
            return QPixmap();
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
            return QPixmap();
        }

        QImage::Format qimageFormat = static_cast<QImage::Format>(imageFormat);
        int bitsCountPerPixel = QImage::toPixelFormat(qimageFormat).bitsPerPixel();

        QByteArray fileContent = file.read(imageHeight * imageWidth * bitsCountPerPixel / 8);
        QImage image(reinterpret_cast<uchar *>(fileContent.data()), imageWidth, imageHeight, imageStride, qimageFormat);
        // close read
        ::close(fd[0]);
        return QPixmap::fromImage(image);
    }
};

X11WindowPreviewContainer::X11WindowPreviewContainer(X11WindowMonitor* monitor, QWidget *parent)
    : DBlurEffectWidget(parent)
    , m_direction(0)
    , m_isPreviewEntered(false)
    , m_isDockPreviewCount(0)
    , m_model(new AppItemWindowModel(this))
{
    m_hideTimer = new QTimer(this);
    m_hideTimer->setSingleShot(true);
    m_hideTimer->setInterval(300);

    setWindowFlags(Qt::ToolTip | Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus | Qt::FramelessWindowHint);
    setMouseTracking(true);
    initUI();

    connect(m_hideTimer, &QTimer::timeout, this, &X11WindowPreviewContainer::callHide);

    connect(m_closeAllButton, &DIconButton::clicked, this, [this](){
        if (m_previewItem.isNull()) return;
        for (auto window : m_previewItem->getAppendWindows()) {
            window->close();
        }
    });

    connect(m_view, &DListView::entered, this, [this](const QModelIndex &enter){
        m_closeAllButton->setVisible(false);
        if (WM_HELPER->hasComposite()) {
            m_monitor->previewWindow(enter.data(WindowIdRole).toInt());
        }

        QPixmap pix;
        const QStringList strs = enter.data(WindowIconRole).toString().split("base64,");

        if (strs.size() == 2) {
            pix.loadFromData(QByteArray::fromBase64(strs.at(1).toLatin1()));
            pix = pix.scaled(PREVIEW_TITLE_HEGIHT, PREVIEW_TITLE_HEGIHT);
        }

        if (!pix.isNull()) {
            m_previewIcon->setPixmap(pix);
        } else {
            m_previewIcon->setPixmap(QIcon::fromTheme(m_previewItem->icon()).pixmap(PREVIEW_TITLE_HEGIHT, PREVIEW_TITLE_HEGIHT));
        }

        updatePreviewTitle(enter.data(WindowTitleRole).toString());
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
    m_previewIcon->setPixmap(QIcon::fromTheme(item->icon()).pixmap(PREVIEW_TITLE_HEGIHT, PREVIEW_TITLE_HEGIHT));
    updatePreviewTitle(item->name());
    m_model->setData(item);

    updateSize();
    connect(m_previewItem, &AppItem::dataChanged, this, [this](){
        updateSize();
    });

    if (isHidden()) {
        show();
    }
}

void X11WindowPreviewContainer::updateOrientation()
{

    if (m_direction % 2 == 0 && WM_HELPER->hasComposite()) {
        m_view->setFlow(QListView::LeftToRight);
    } else {
        m_view->setFlow(QListView::TopToBottom);
    }
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
    updateOrientation();
    return DBlurEffectWidget::showEvent(event);
}

void X11WindowPreviewContainer::hideEvent(QHideEvent*)
{
    m_previewItem->disconnect(this);
    m_previewItem = QPointer<AppItem>(nullptr);
}

void X11WindowPreviewContainer::resizeEvent(QResizeEvent *event)
{
    m_previewTitle->setText(
        QFontMetrics(m_previewTitle->font()).elidedText(m_previewTitleStr, Qt::TextElideMode::ElideRight, width() - m_previewIcon->width() - m_closeAllButton->width() - 25)
    );

    updatePosition();
}

void X11WindowPreviewContainer::updatePosition()
{
    auto screenRect = screen()->geometry();
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

    xPosition = std::max(xPosition, screenRect.x() + 10);
    xPosition = std::min(xPosition, screenRect.x() + screenRect.width() - width() - 10);

    yPosition = std::max(yPosition, screenRect.y() + 10);
    yPosition = std::min(yPosition, screenRect.y() + screenRect.height() - height() - 10);

    move(xPosition, yPosition);
}

void X11WindowPreviewContainer::updatePreviewTitle(const QString& title)
{
    m_previewTitleStr = title;
    m_previewTitle->setText(
        QFontMetrics(m_previewTitle->font()).elidedText(m_previewTitleStr, Qt::TextElideMode::ElideRight, width() - m_previewIcon->width() - m_closeAllButton->width() - 25)
    );
}

void X11WindowPreviewContainer::initUI()
{
    m_view = new QListView;
    QVBoxLayout* mainLayout = new QVBoxLayout;
    QHBoxLayout* titleLayout = new QHBoxLayout;
    titleLayout->setContentsMargins(11, 0, 5, 0);

    m_previewIcon = new QLabel;
    m_previewTitle = new QLabel;
    m_previewIcon->setFixedSize(PREVIEW_TITLE_HEGIHT, PREVIEW_TITLE_HEGIHT);

    m_closeAllButton = new DIconButton;
    m_closeAllButton->setFixedSize(PREVIEW_TITLE_HEGIHT, PREVIEW_TITLE_HEGIHT);
    m_closeAllButton->setIcon(QIcon::fromTheme("close"));
    m_closeAllButton->setFlat(true);

    m_previewIcon->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_previewTitle->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    auto updateWindowTitleColorType = [this](){
        QPalette pa = palette();
        auto type = DGuiApplicationHelper::instance()->themeType();
        pa.setColor(QPalette::WindowText, type == DGuiApplicationHelper::ColorType::LightType ? Qt::black : Qt::white);
        m_previewTitle->setPalette(pa);
    };

    updateWindowTitleColorType();

    connect(DGuiApplicationHelper::instance(), & DGuiApplicationHelper::themeTypeChanged, this , updateWindowTitleColorType);

    titleLayout->addWidget(m_previewIcon);
    titleLayout->addWidget(m_previewTitle);
    titleLayout->addStretch();
    titleLayout->addWidget(m_closeAllButton);

    m_view->setModel(m_model);
    m_view->setItemDelegate(new AppItemWindowDeletegate(m_view, this));
    m_view->setSpacing(1);
    m_view->setMouseTracking(true);
    m_view->viewport()->installEventFilter(this);
    m_view->setAutoFillBackground(false);
    m_view->setFrameStyle(QFrame::NoFrame);
    m_view->setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);

    QPalette pal = m_view->palette();
    pal.setColor(QPalette::Base, Qt::transparent);
    m_view->setPalette(pal);

    mainLayout->addLayout(titleLayout);
    mainLayout->addWidget(m_view);
    setLayout(mainLayout);
}

void X11WindowPreviewContainer::updateSize()
{
    auto screenSize = screen()->size();
    int height = 0, width = 0;

    if (WM_HELPER->hasComposite()) {
        if (m_direction % 2 == 0) {
            width = (PREVIEW_CONTENT_WIDTH + 2) * m_previewItem->getAppendWindows().size() + 20;
            height = 168;
        } else {
            width = 260;
            height = (PREVIEW_CONTENT_HEIGHT + 2) * m_previewItem->getAppendWindows().size() + 25 + PREVIEW_TITLE_HEGIHT;
        }
    } else {
        height = (PREVIEW_TITLE_HEGIHT + 10) * m_previewItem->getAppendWindows().size() + 25 + PREVIEW_TITLE_HEGIHT;
        width = 260;
    }

    screenSize -= QSize(m_direction % 2 == 0 ? 0 : m_baseWindow->width(), m_direction % 2 == 0 ? m_baseWindow->height() : 0);
    setFixedSize(std::min(width, screenSize.width() - 20), std::min(height, screenSize.height() - 20));
}

bool X11WindowPreviewContainer::eventFilter(QObject *watched, QEvent *event)
{
    if (watched != m_view->viewport()) return false;

    switch (event->type()) {
    case QEvent::HoverLeave: {
        if (WM_HELPER->hasComposite()) {
            m_monitor->cancelPreviewWindow();
        }

        m_closeAllButton->setVisible(true);
        if (m_previewItem.isNull()) return false;

        m_previewIcon->setPixmap(QIcon::fromTheme(m_previewItem->icon()).pixmap(PREVIEW_TITLE_HEGIHT, PREVIEW_TITLE_HEGIHT));
        updatePreviewTitle(m_previewItem->name());
        break;
    }
    case QEvent::QEvent::MouseButtonRelease: {
        auto mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() != Qt::LeftButton) return false;

        auto index = m_view->indexAt(mouseEvent->pos());
        m_previewItem->getAppendWindows()[index.row()]->activate();
        DBlurEffectWidget::hide();
        break;
    }
    default: {}
    }

    return false;
}
}

