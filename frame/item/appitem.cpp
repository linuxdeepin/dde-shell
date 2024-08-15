// SPDX-FileCopyrightText: 2011 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "appitem.h"
#include "themeappicon.h"
#include "xcb_misc.h"
#include "appswingeffectbuilder.h"
#include "utils.h"
#include "settingmanager.h"

#include <dde-api/eventlogger.hpp>

#include <X11/X.h>
#include <X11/Xlib.h>

#include <QPainter>
#include <QDrag>
#include <QMouseEvent>
#include <QApplication>
#include <QHBoxLayout>
#include <QTimeLine>
#include <QX11Info>
#include <QGSettings>

#include <DGuiApplicationHelper>

DGUI_USE_NAMESPACE
DCORE_USE_NAMESPACE

#define APP_DRAG_THRESHOLD      20

QPoint AppItem::MousePressPos;

AppItem::AppItem(const QGSettings *appSettings, const QGSettings *activeAppSettings, const QGSettings *dockedAppSettings, const QDBusObjectPath &entry, QWidget *parent)
    : DockItem(ItemType_App, parent)
    , m_appSettings(appSettings)
    , m_activeAppSettings(activeAppSettings)
    , m_dockedAppSettings(dockedAppSettings)
    , m_appPreviewTips(nullptr)
    , m_itemEntryInter(new DockEntryInter("com.deepin.dde.daemon.Dock", entry.path(), QDBusConnection::sessionBus(), this))
    , m_swingEffectView(nullptr)
    , m_itemAnimation(nullptr)
    , m_drag(nullptr)
    , m_retryTimes(0)
    , m_iconValid(true)
    , m_lastclickTimes(0)
    , m_needTip(true)
    , m_inOverflow(false)
    , m_needChangedHidePos(false)
    , m_overflowItemRect(QRect())
    , m_appIcon(QPixmap())
    , m_updateIconGeometryTimer(new QTimer(this))
    , m_retryObtainIconTimer(new QTimer(this))
    , m_refershIconTimer(new QTimer(this))
    , m_themeType(DGuiApplicationHelper::instance()->themeType())
    , m_entry(entry)
{
    auto *centralLayout = new QHBoxLayout;
    centralLayout->setMargin(0);
    centralLayout->setSpacing(0);

    setObjectName(m_itemEntryInter->name());
    setAcceptDrops(true);
    setLayout(centralLayout);

    m_id = m_itemEntryInter->id();
    m_active = m_itemEntryInter->isActive();
    m_currentWindowId = m_itemEntryInter->currentWindow();

    m_updateIconGeometryTimer->setInterval(50);
    m_updateIconGeometryTimer->setSingleShot(true);

    m_retryObtainIconTimer->setInterval(3000);
    m_retryObtainIconTimer->setSingleShot(true);

    m_refershIconTimer->setInterval(1000);
    m_refershIconTimer->setSingleShot(false);

    connect(m_itemEntryInter, &DockEntryInter::IsActiveChanged, this, &AppItem::activeChanged);
    connect(m_itemEntryInter, &DockEntryInter::IsActiveChanged, this, static_cast<void (AppItem::*)()>(&AppItem::update));
    connect(m_itemEntryInter, &DockEntryInter::WindowInfosChanged, this, &AppItem::updateWindowInfos, Qt::QueuedConnection);
    connect(m_itemEntryInter, &DockEntryInter::IconChanged, this, &AppItem::refreshIcon);

    connect(m_retryObtainIconTimer, &QTimer::timeout, this, &AppItem::refreshIcon, Qt::QueuedConnection);
    connect(m_updateIconGeometryTimer, &QTimer::timeout, this, &AppItem::updateWindowIconGeometries, Qt::QueuedConnection);
    connect(this, &AppItem::requestUpdateEntryGeometries, this, &AppItem::updateWindowIconGeometries);
    connect(DWindowManagerHelper::instance(), &DWindowManagerHelper::hasBlurWindowChanged, this, [this] {
        hidePopup();
        // showHoverTips定时器设置了500ms，其他地方hidepopup会终止定时器，导致这里启动定时器后，被其他地方关掉了，所以这里延时500
        QTimer::singleShot(500, this, [this] {
            if (underMouse()) {
                showHoverTips();
            }
        });
    });

    updateWindowInfos(m_itemEntryInter->windowInfos());
    refreshIcon();

    if (m_appSettings)
        connect(m_appSettings, &QGSettings::changed, this, &AppItem::onGSettingsChanged);
    if (m_dockedAppSettings)
        connect(m_dockedAppSettings, &QGSettings::changed, this, &AppItem::onGSettingsChanged);
    if (m_activeAppSettings)
        connect(m_activeAppSettings, &QGSettings::changed, this, &AppItem::onGSettingsChanged);

    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &AppItem::onThemeTypeChanged);

    /** 日历 1S定时判断是否刷新icon的处理 */
    connect(m_refershIconTimer, &QTimer::timeout, this, &AppItem::onRefreshIcon);

    m_repeatPresentWindowTime.start();
}

/**将属于同一个应用的窗口合并到同一个应用图标
 * @brief AppItem::checkEntry
 */
void AppItem::checkEntry()
{
    m_itemEntryInter->Check();
}

const QString AppItem::appId() const
{
    return m_id;
}

bool AppItem::isValid() const
{
    return m_itemEntryInter->isValid() && !m_itemEntryInter->id().isEmpty();
}

// Update _NET_WM_ICON_GEOMETRY property for windows that every item
// that manages, so that WM can do proper animations for specific
// window behaviors like minimization.
void AppItem::updateWindowIconGeometries()
{
    // 普通应用，使用自身item位置
    // 溢出应用，并且在溢出区未展开时，最小化的位置需要改变为溢出区按键的位置；溢出区展开时，使用自身item位置
    QRect r = m_inOverflow && m_needChangedHidePos ? m_overflowItemRect : QRect(mapToGlobal(QPoint(0, 0)), mapToGlobal(QPoint(width(), height())));

    if (Utils::IS_WAYLAND_DISPLAY) {
        Q_EMIT requestUpdateItemMinimizedGeometry(r);
        return;
    }

    if (!QX11Info::connection()) {
        qCWarning(DOCK_APP) << "Update window icon geometries, QX11 info connection is null";
        return;
    }

    auto *xcb_misc = XcbMisc::instance();

    for (auto it(m_windowInfos.cbegin()); it != m_windowInfos.cend(); ++it)
        xcb_misc->set_window_icon_geometry(it.key(), r);
}

void AppItem::updateOverflowAppMinimizedGeometry(bool overflow, bool needChange, const QRect &rect)
{
    m_inOverflow = overflow;
    m_needChangedHidePos = needChange;
    m_overflowItemRect = rect;
    updateWindowIconGeometries();
}

/**取消驻留在dock上的应用
 * @brief AppItem::undock
 */
void AppItem::undock()
{
    m_itemEntryInter->RequestUndock();
}

QWidget *AppItem::appDragWidget()
{
    if (m_drag) {
        return m_drag->appDragWidget();
    }

    return nullptr;
}

void AppItem::setDockInfo(Dock::Position dockPosition, const QRect &dockGeometry)
{
    if (m_drag) {
        m_drag->appDragWidget()->setDockInfo(dockPosition, dockGeometry);
    }
}

QString AppItem::appIconName() const
{
    return m_itemEntryInter->icon();
}

QString AppItem::accessibleName()
{
    return m_itemEntryInter->name();
}

quint32 AppItem::getActiveWindowId() const
{
    if (!m_itemEntryInter)
        return m_currentWindowId;

    return m_itemEntryInter->currentWindow();
}

void AppItem::moveEvent(QMoveEvent *e)
{
    DockItem::moveEvent(e);

    if (m_drag) {
        m_drag->appDragWidget()->setOriginPos(mapToGlobal(appIconPosition()));
    }

    m_updateIconGeometryTimer->start();
}

void AppItem::paintEvent(QPaintEvent *e)
{
    DockItem::paintEvent(e);
    if (m_dragging || (m_swingEffectView != nullptr && DockDisplayMode != Fashion))
        return;

    QPainter painter(this);
    if (!painter.isActive())
        return;
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    const QRectF itemRect = rect();

    if (DockDisplayMode == Efficient) {
        // draw background
        qreal min = qMin(itemRect.width(), itemRect.height());
        QRectF backgroundRect = QRectF(itemRect.x(), itemRect.y(), min, min);

        // 图标之间的背景底色间距需要随着任务栏的变化而动态改变，避免当任务栏变大时，间距看起来很小；
        // 改变策略为分三挡变化，当前itemRect大小范围是[36~96]，故三挡间距为：[36~54]：2, (54~72]：3, (72~96]:4
        const int margin = min > 72 ? 4 : ( min > 54 ? 3 : 2);
        backgroundRect = backgroundRect.marginsRemoved(QMargins(margin, margin, margin, margin));
        backgroundRect.moveCenter(itemRect.center());

        QPainterPath path;
        path.addRoundedRect(backgroundRect, 8, 8);

        if (m_active) {
            if (DGuiApplicationHelper::DarkType == m_themeType) {
                painter.fillPath(path, QColor(255, 255, 255, 255 * 0.6));
                painter.setPen(QColor(0, 0, 0, 255 * 0.2));
            } else {
                painter.fillPath(path, QColor(0, 0, 0, 255 * 0.8));
                painter.setPen(QColor(255, 255, 255, 255 * 0.2));
            }
            painter.drawPath(path);
        } else if (!m_windowInfos.isEmpty()) {
            if (hasAttention()) {
                painter.fillPath(path, QColor(241, 138, 46, 255 * .8));
            } else {
                if (DGuiApplicationHelper::DarkType == m_themeType) {
                    painter.fillPath(path, QColor(255, 255, 255, 255 * 0.2));
                    painter.setPen(QColor(0, 0, 0, 255 * 0.2));
                } else {
                    painter.fillPath(path, QColor(0, 0, 0, 255 * 0.2));
                    painter.setPen(QColor(255, 255, 255, 255 * 0.2));
                }
                painter.drawPath(path);
            }
        }
    } else {
        if (!m_windowInfos.isEmpty()) {
            QPoint p;
            QPixmap pixmap;
            QPixmap activePixmap;
            if (DGuiApplicationHelper::DarkType == m_themeType) {
                m_horizontalIndicator = QPixmap(":/indicator/resources/indicator_dark.svg");
                m_verticalIndicator = QPixmap(":/indicator/resources/indicator_dark_ver.svg");
            } else {
                m_horizontalIndicator = QPixmap(":/indicator/resources/indicator.svg");
                m_verticalIndicator = QPixmap(":/indicator/resources/indicator_ver.svg");
            }
            m_activeHorizontalIndicator = QPixmap(":/indicator/resources/indicator_active.svg");
            m_activeVerticalIndicator = QPixmap(":/indicator/resources/indicator_active_ver.svg");
            switch (DockPosition) {
            case Top:
                pixmap = m_horizontalIndicator;
                activePixmap = m_activeHorizontalIndicator;
                p.setX((itemRect.width() - pixmap.width()) / 2);
                p.setY(1);
                break;
            case Bottom:
                pixmap = m_horizontalIndicator;
                activePixmap = m_activeHorizontalIndicator;
                p.setX((itemRect.width() - pixmap.width()) / 2);
                p.setY(itemRect.height() - pixmap.height() - 1);
                break;
            case Left:
                pixmap = m_verticalIndicator;
                activePixmap = m_activeVerticalIndicator;
                p.setX(1);
                p.setY((itemRect.height() - pixmap.height()) / 2);
                break;
            case Right:
                pixmap = m_verticalIndicator;
                activePixmap = m_activeVerticalIndicator;
                p.setX(itemRect.width() - pixmap.width() - 1);
                p.setY((itemRect.height() - pixmap.height()) / 2);
                break;
            }

            if (m_active)
                painter.drawPixmap(p, activePixmap);
            else
                painter.drawPixmap(p, pixmap);
        }
    }

    if (m_swingEffectView != nullptr)
        return;

    // icon
    if (m_appIcon.isNull())
        return;

    painter.drawPixmap(appIconPosition(), m_appIcon);
}

void AppItem::mouseReleaseEvent(QMouseEvent *e)
{
    if (checkGSettingsControl()) {
        return;
    }

    // 获取时间戳qint64转quint64，是不存在任何问题的
    quint64 curTimestamp = QDateTime::currentDateTime().toMSecsSinceEpoch();
    if ((curTimestamp - m_lastclickTimes) < 300)
        return;

    m_lastclickTimes = curTimestamp;

    // 鼠标在图标外边松开时，没必要响应点击操作
    QSize Size = size();
    qCDebug(DOCK_APP) << "Dock size: " << Size << ", map from global: " << mapFromGlobal(QCursor::pos())
             << ", cursor pos: " << QCursor::pos();
    const QRect rect { QPoint(0, 0), Size};
    QPoint globalPos = mapFromGlobal(QCursor::pos());
    if (!(globalPos.x() < 0 || globalPos.y() < 0) && !rect.contains(globalPos)) {
        qCWarning(DOCK_APP) << "Global pos out of dock size, dock size: " << Size << ", global pos: " << globalPos;
        return;
    }

    if (e->button() == Qt::MiddleButton) {
        m_itemEntryInter->NewInstance(QX11Info::getTimestamp());

        // play launch effect
        if (m_windowInfos.isEmpty())
            playSwingEffect();

    } else if (e->button() == Qt::LeftButton) {
        if (checkAndResetTapHoldGestureState() && e->source() == Qt::MouseEventSynthesizedByQt) {
            qCDebug(DOCK_APP) << "Tap and hold gesture detected, ignore the synthesized mouse release event";
            return;
        }

        qCDebug(DOCK_APP) << "App item clicked, name:" << m_itemEntryInter->name()
                 << ", item entry interface id:" << m_itemEntryInter->id() << ", app id:" << m_id << ", icon:" << m_itemEntryInter->icon();

        if (m_itemEntryInter->windowInfos().empty()) {
            QString name = m_itemEntryInter->name();
            DDE_EventLogger::EventLoggerData data;
            data.tid = EVENT_LOGGER_DOCK_START_APP;
            data.event = "dock_app_click";
            data.target = name;
            data.message = {{"appName", name}, {"appDesktop", m_itemEntryInter->desktopFile()}};
            DDE_EventLogger::EventLogger::instance().writeEventLog(data);
        }

        m_itemEntryInter->Activate(QX11Info::getTimestamp());

        // play launch effect
        if (m_windowInfos.isEmpty() && DGuiApplicationHelper::isSpecialEffectsEnvironment())
            playSwingEffect();

        emit hideOverflowPopup();
    }
    DockItem::mouseReleaseEvent(e);
}

void AppItem::mousePressEvent(QMouseEvent *e)
{
    if (checkGSettingsControl()) {
        return;
    }
    m_updateIconGeometryTimer->stop();
    hidePopup();
    onResetPreview();

    if (e->button() == Qt::LeftButton)
        MousePressPos = e->pos();

    // context menu will handle in DockItem
    DockItem::mousePressEvent(e);
}

void AppItem::mouseMoveEvent(QMouseEvent *e)
{
    e->accept();

    // handle drag
    if (e->buttons() != Qt::LeftButton)
        return;

    const QPoint pos = e->pos();
    if (!rect().contains(pos))
        return;
}

void AppItem::presentWindows() {
    if (m_repeatPresentWindowTime.elapsed() > 300) {
        m_repeatPresentWindowTime.restart();
        m_itemEntryInter->PresentWindows();
    }
}

bool AppItem::cursorInPreviewClose() const
{
    if (!m_appPreviewTips)
        return false;

    return m_appPreviewTips->cursorInClose();
}

void AppItem::resizeEvent(QResizeEvent *e)
{
    DockItem::resizeEvent(e);

    stopSwingEffect();

    refreshIcon();
}

void AppItem::dragEnterEvent(QDragEnterEvent *e)
{
    if (checkGSettingsControl()) {
        return;
    }

    // ignore drag from panel
    if (e->source()) {
        return e->ignore();
    }

    // ignore request dock event
    QString draggingMimeKey = e->mimeData()->formats().contains("RequestDock") ? "RequestDock" : "text/plain";
    if (QMimeDatabase().mimeTypeForFile(e->mimeData()->data(draggingMimeKey)).name() == "application/x-desktop") {
        return e->ignore();
    }

    e->accept();
}

void AppItem::dragMoveEvent(QDragMoveEvent *e)
{
    if (checkGSettingsControl()) {
        return;
    }

    DockItem::dragMoveEvent(e);

    if (m_windowInfos.isEmpty())
        return;

    if (!PopupWindow->isVisible(this) || !m_appPreviewTips)
        showPreview();
}

void AppItem::dropEvent(QDropEvent *e)
{
    QStringList uriList;
    const auto &mimeUrls = e->mimeData()->urls();
    for (const auto &uri : mimeUrls) {
        uriList << uri.toEncoded();
    }

    qCDebug(DOCK_APP) << "Accept drop event with URIs: " << uriList;
    m_itemEntryInter->HandleDragDrop(QX11Info::getTimestamp(), uriList);
}

void AppItem::leaveEvent(QEvent *e)
{
    m_hover = false;
    m_popupTipsDelayTimer->stop();

    if (m_appPreviewTips) {
        if (m_appPreviewTips->isVisible())
            return m_appPreviewTips->prepareHide();
    }
    DockItem::leaveEvent(e);
}

void AppItem::showHoverTips()
{
    if (checkGSettingsControl() || !m_needTip  || qApp->property(PRESENTWINDOW).toBool() || qApp->property(APP_DRAG_STATE).toBool()) {
        return;
    }

    if (!m_windowInfos.isEmpty())
        return showPreview();

    DockItem::showHoverTips();
}

void AppItem::invokedMenuItem(const QString &itemId, const bool checked)
{
    Q_UNUSED(checked);

    m_itemEntryInter->HandleMenuItem(QX11Info::getTimestamp(), itemId);
}

const QString AppItem::contextMenu() const
{
    return m_itemEntryInter->menu();
}

QWidget *AppItem::popupTips()
{
    if (checkGSettingsControl())
        return nullptr;

    if (m_dragging)
        return nullptr;

    static QPointer<TipsWidget> appNameTips;
    if (appNameTips.isNull()) {
        appNameTips = new TipsWidget(topLevelWidget());
    }
    appNameTips->setAccessibleName("tip");
    appNameTips->setObjectName(m_itemEntryInter->name());

    if (!m_windowInfos.isEmpty()) {
        Q_ASSERT(m_windowInfos.contains(m_currentWindowId));
        appNameTips->setText(m_windowInfos[m_currentWindowId].title.simplified());
    } else {
        appNameTips->setText(m_itemEntryInter->name().simplified());
    }

    return appNameTips.data();
}

bool AppItem::hasAttention() const
{
    auto it = std::find_if(m_windowInfos.constBegin(), m_windowInfos.constEnd(), [ = ] (const auto &info) {
        return info.attention;
    });

    return (it != m_windowInfos.end());
}

bool AppItem::hasWindows() const
{
    return !m_windowInfos.isEmpty();
}

QPoint AppItem::appIconPosition() const
{
    const auto ratio = devicePixelRatioF();
    const QRectF itemRect = rect();
    const QRectF iconRect = m_appIcon.rect();
    const qreal iconX = itemRect.center().x() - iconRect.center().x() / ratio;
    const qreal iconY = itemRect.center().y() - iconRect.center().y() / ratio;

    return QPoint(iconX, iconY);
}

void AppItem::showPopupWindow(QWidget *const content, const bool model, const int radius)
{
    if(qApp->property(PRESENTWINDOW).toBool())
        return;

    m_popupShown = true;
    m_lastPopupWidget = content;

    if (model)
        emit requestWindowAutoHide(false);

    DockPopupWindow *popup = PopupWindow->toTipsPopup(this);
    popup->setPopupRadius(radius);

    // 设置预览界面是否开启左右两边的圆角
    QWidget *lastContent = popup->getContent();
    if (lastContent)
        lastContent->setVisible(false);

    popup->resize(content->sizeHint());
    popup->setContent(content);

    const QPoint p = popupMarkPoint();
    if (!popup->isVisible())
        QMetaObject::invokeMethod(popup, "show", Qt::QueuedConnection, Q_ARG(QPoint, p), Q_ARG(bool, model));
    else
        popup->show(p, model);

    connect(popup, &DockPopupWindow::accept, this, &AppItem::popupWindowAccept, Qt::UniqueConnection);
}

void AppItem::updateWindowInfos(const WindowInfoMap &info)
{
    m_windowInfos = info;
    m_currentWindowId = info.isEmpty() ? 0 :  info.firstKey();
    if (m_appPreviewTips)
        m_appPreviewTips->setWindowInfos(m_windowInfos, m_itemEntryInter->GetAllowedCloseWindows().value());
    m_updateIconGeometryTimer->start();

    // process attention effect
    if (hasAttention()) {
        if (DockDisplayMode == DisplayMode::Fashion)
            playSwingEffect();
    } else {
        stopSwingEffect();
    }
    update();

    if (m_currentWindowId == 0) {
        emit requestCloseWindow();
    }
}

void AppItem::updateSnapSize()
{
    if (m_appPreviewTips){
        m_appPreviewTips->fetchSnapshots();
    }
    update();
}

void AppItem::refreshIcon()
{
    if (!isVisible())
        return;

    const QString icon = m_itemEntryInter->icon();

    if (icon.isNull()) {
        return;
    }

    const int iconSize = qMin(width(), height());

    if (DockDisplayMode == Efficient)
        m_iconValid = ThemeAppIcon::getIcon(m_appIcon, icon, iconSize * 0.7, !m_iconValid);
    else
        m_iconValid = ThemeAppIcon::getIcon(m_appIcon, icon, iconSize * 0.8, !m_iconValid);

    if (!m_refershIconTimer->isActive() && m_itemEntryInter->icon() == "dde-calendar") {
        m_refershIconTimer->start();
    }

    if (!m_iconValid) {
        if (m_retryTimes < 10) {
            m_retryTimes++;
            qCDebug(DOCK_APP) << m_itemEntryInter->name() << "obtain app icon(" << icon << ")failed, retry times:" << m_retryTimes;
            // Maybe the icon was installed after we loaded the caches.
            // QIcon::setThemeSearchPaths will force Qt to re-check the gtk cache validity.
            QIcon::setThemeSearchPaths(QIcon::themeSearchPaths());

            m_retryObtainIconTimer->start();
        } else {
            // 如果图标获取失败，一分钟后再自动刷新一次（如果还是显示异常，基本需要应用自身看下为什么了）
            if (!m_iconValid)
                QTimer::singleShot(60 * 1000, this, &AppItem::refreshIcon);
        }

        update();

        return;
    } else if (m_retryTimes > 0) {
        // reset times
        m_retryTimes = 0;
    }

    update();

    m_updateIconGeometryTimer->start();
}

void AppItem::onRefreshIcon()
{
    if (QDate::currentDate() == m_curDate)
        return;

    m_curDate = QDate::currentDate();
    refreshIcon();
}

void AppItem::onResetPreview()
{
    emit requestSetShowWindowPreviewState(false);
    if (m_appPreviewTips != nullptr) {
        m_appPreviewTips->deleteLater();
        m_appPreviewTips = nullptr;
        disconnect(m_itemEntryInter, &DockEntryInter::WindowInfosChanged, this, &AppItem::updateSnapSize);
    }
}

void AppItem::activeChanged()
{
    m_active = !m_active;
    if (m_active) {
        emit requestActive();
    }
}

void AppItem::showPreview()
{
    if (m_windowInfos.isEmpty())
        return;

    emit requestSetShowWindowPreviewState(true);

    m_appPreviewTips = new PreviewContainer;
    m_appPreviewTips->updateDockSize(DockSize);
    m_appPreviewTips->setWindowInfos(m_windowInfos, m_itemEntryInter->GetAllowedCloseWindows().value());
    m_appPreviewTips->fetchSnapshots();
    m_appPreviewTips->updateLayoutDirection(DockPosition);

    connect(m_appPreviewTips, &PreviewContainer::requestActivateWindow, this, &AppItem::requestActivateWindow, Qt::QueuedConnection);
    connect(m_appPreviewTips, &PreviewContainer::requestPreviewWindow, this, &AppItem::requestPreviewWindow, Qt::QueuedConnection);
    connect(m_appPreviewTips, &PreviewContainer::requestCancelPreviewWindow, this, &AppItem::requestCancelPreview);
    connect(m_appPreviewTips, &PreviewContainer::requestHidePopup, this, &AppItem::hidePopup);
    connect(m_appPreviewTips, &PreviewContainer::requestCheckWindows, m_itemEntryInter, &DockEntryInter::Check);

    connect(m_appPreviewTips, &PreviewContainer::requestActivateWindow, this, &AppItem::onResetPreview);
    connect(m_appPreviewTips, &PreviewContainer::requestCancelPreviewWindow, this, &AppItem::onResetPreview);
    connect(m_appPreviewTips, &PreviewContainer::requestHidePopup, this, &AppItem::onResetPreview);
    connect(m_itemEntryInter, &DockEntryInter::WindowInfosChanged, this, &AppItem::updateSnapSize, Qt::QueuedConnection);

    // 预览标题显示方式的配置
    m_appPreviewTips->setTitleDisplayMode(SettingManager::instance()->showWindowName());
    showPopupWindow(m_appPreviewTips, false, 18);
}

void AppItem::playSwingEffect()
{
    // NOTE(sbw): return if animation view already playing
    if (m_swingEffectView != nullptr)
        return;

    if (rect().isEmpty())
        return checkAttentionEffect();

    stopSwingEffect();

    QPair<QGraphicsView *, QGraphicsItemAnimation *> pair =  SwingEffect(
                this, m_appIcon, rect(), devicePixelRatioF());

    m_swingEffectView = pair.first;
    m_itemAnimation = pair.second;

    QTimeLine *tl = m_itemAnimation->timeLine();
    connect(tl, &QTimeLine::stateChanged, this, [ = ](QTimeLine::State newState) {
        if (newState == QTimeLine::NotRunning) {
            m_swingEffectView->hide();
            layout()->removeWidget(m_swingEffectView);
            m_swingEffectView = nullptr;
            m_itemAnimation = nullptr;
            checkAttentionEffect();
        }
    });

    layout()->addWidget(m_swingEffectView);
    tl->start();
}

void AppItem::stopSwingEffect()
{
    if (m_swingEffectView == nullptr || m_itemAnimation == nullptr)
        return;

    // stop swing effect
    m_swingEffectView->setVisible(false);

    if (m_itemAnimation->timeLine() && m_itemAnimation->timeLine()->state() != QTimeLine::NotRunning)
        m_itemAnimation->timeLine()->stop();
}

void AppItem::checkAttentionEffect()
{
    QTimer::singleShot(1000, this, [ = ] {
        if (DockDisplayMode == DisplayMode::Fashion && hasAttention())
            playSwingEffect();
    });
}

void AppItem::onGSettingsChanged(const QString &key)
{
    if (key != "enable") {
        return;
    }

    const QGSettings *setting = m_itemEntryInter->isDocked()
            ? m_dockedAppSettings
            : m_activeAppSettings;

    if (setting && setting->keys().contains("enable")) {
        const bool isEnable = !m_appSettings || (m_appSettings->keys().contains("enable") && m_appSettings->get("enable").toBool());
        setVisible(isEnable && setting->get("enable").toBool());
    }
}

bool AppItem::checkGSettingsControl() const
{
    const QGSettings *setting = m_itemEntryInter->isDocked()
            ? m_dockedAppSettings
            : m_activeAppSettings;

    return ((m_appSettings && m_appSettings->keys().contains("control") && m_appSettings->get("control").toBool())
            || (setting && setting->keys().contains("control") && setting->get("control").toBool()));
}

void AppItem::onThemeTypeChanged(DGuiApplicationHelper::ColorType themeType)
{
    m_themeType = themeType;
    update();
}

// 放到最下面是因为析构函数和匿名函数会影响lcov统计单元测试的覆盖率
AppItem::~AppItem()
{
    stopSwingEffect();
}

void AppItem::showEvent(QShowEvent *e)
{
    DockItem::showEvent(e);

    QTimer::singleShot(0, this, [ = ] {
        onGSettingsChanged("enable");
    });

    refreshIcon();
}
