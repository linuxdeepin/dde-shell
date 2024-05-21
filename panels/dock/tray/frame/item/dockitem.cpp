// Copyright (C) 2011 ~ 2018 Deepin Technology Co., Ltd.
// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "dockitem.h"
#include "pluginsitem.h"
#include "utils.h"

#include <QMouseEvent>
#include <QJsonObject>
#include <QCursor>
#include <QApplication>
#include <QMenu>

#define PLUGIN_MARGIN  10
#define ITEM_MAXSIZE    100

Position DockItem::DockPosition = Position::Top;
DisplayMode DockItem::DockDisplayMode = DisplayMode::Efficient;
QPointer<DockPopupWindow> DockItem::PopupWindow(nullptr);

DockItem::DockItem(QWidget *parent)
    : QWidget(parent)
    , m_hover(false)
    , m_popupShown(false)
    , m_tapAndHold(false)
    , m_draging(false)
    , m_contextMenu(new QMenu(this))
    , m_popupTipsDelayTimer(new QTimer(this))
    , m_popupAdjustDelayTimer(new QTimer(this))
{
    if (PopupWindow.isNull()) {
        DockPopupWindow *blurRectangle = new DockPopupWindow(nullptr);
        blurRectangle->setRadius(18);
        blurRectangle->setObjectName("apppopup");
        if (Utils::IS_WAYLAND_DISPLAY) {
            Qt::WindowFlags flags = blurRectangle->windowFlags() | Qt::FramelessWindowHint;
            blurRectangle->setWindowFlags(flags);
        }
        PopupWindow = blurRectangle;
        connect(qApp, &QApplication::aboutToQuit, PopupWindow, &DockPopupWindow::deleteLater);
    }

    m_popupTipsDelayTimer->setInterval(500);
    m_popupTipsDelayTimer->setSingleShot(true);

    m_popupAdjustDelayTimer->setInterval(10);
    m_popupAdjustDelayTimer->setSingleShot(true);

    connect(m_popupTipsDelayTimer, &QTimer::timeout, this, &DockItem::showHoverTips);
    connect(m_popupAdjustDelayTimer, &QTimer::timeout, this, &DockItem::updatePopupPosition, Qt::QueuedConnection);
    connect(m_contextMenu, &QMenu::triggered, this, &DockItem::menuActionClicked);

    grabGesture(Qt::TapAndHoldGesture);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

DockItem::~DockItem()
{
    if (m_popupShown)
        popupWindowAccept();
}

QSize DockItem::sizeHint() const
{
    int size = qMin(qMin(maximumWidth(), maximumHeight()), ITEM_MAXSIZE);

    return QSize(size, size);
}

QString DockItem::accessibleName()
{
    return QString();
}

void DockItem::setDockPosition(const Position side)
{
    DockPosition = side;
}

void DockItem::setDockDisplayMode(const DisplayMode mode)
{
    DockDisplayMode = mode;
}

void DockItem::gestureEvent(QGestureEvent *event)
{
    if (!event)
        return;

    QGesture *gesture = event->gesture(Qt::TapAndHoldGesture);

    if (!gesture)
        return;

    qDebug() << "got TapAndHoldGesture";

    m_tapAndHold = true;
}

bool DockItem::event(QEvent *event)
{
    if (m_popupShown) {
        switch (event->type()) {
        case QEvent::Paint:
            if (!m_popupAdjustDelayTimer->isActive())
                m_popupAdjustDelayTimer->start();
            break;
        default:;
        }
    }

    if (event->type() == QEvent::Gesture)
        gestureEvent(static_cast<QGestureEvent *>(event));

    return QWidget::event(event);
}

void DockItem::updatePopupPosition()
{
    Q_ASSERT(sender() == m_popupAdjustDelayTimer);

    if (!m_popupShown || !PopupWindow->model())
        return;

    if (PopupWindow->getContent() != m_lastPopupWidget.data())
        return popupWindowAccept();

    const QPoint p = popupMarkPoint();
    PopupWindow->show(p, PopupWindow->model());
}

void DockItem::paintEvent(QPaintEvent *e)
{
    QWidget::paintEvent(e);
}

void DockItem::mousePressEvent(QMouseEvent *e)
{
    m_popupTipsDelayTimer->stop();
    hideNonModel();

    if (e->button() == Qt::RightButton) {
        if (perfectIconRect().contains(e->pos())) {
            return showContextMenu();
        }
    }

    // same as e->ignore above
    QWidget::mousePressEvent(e);
}

void DockItem::enterEvent(QEnterEvent *e)
{
    // Remove the bottom area to prevent unintentional operation in auto-hide mode.
    if (!rect().adjusted(0, 0, width(), height() - 5).contains(mapFromGlobal(QCursor::pos()))) {
        return;
    }

    m_hover = true;
    //FIXME: 可能是qt的bug，概率性导致崩溃，待修复
//    m_hoverEffect->setHighlighting(true);

    // 触屏不显示hover效果
    if (!qApp->property(IS_TOUCH_STATE).toBool()) {
        m_popupTipsDelayTimer->start();
    }

    update();

    return QWidget::enterEvent(e);
}

void DockItem::leaveEvent(QEvent *e)
{
    QWidget::leaveEvent(e);

    m_hover = false;
    //FIXME: 可能是qt的bug，概率性导致崩溃，待修复
//    m_hoverEffect->setHighlighting(false);
    m_popupTipsDelayTimer->stop();

    // auto hide if popup is not model window
    if (m_popupShown && !PopupWindow->model())
        hidePopup();

    update();
}

const QRect DockItem::perfectIconRect() const
{
    const QRect itemRect = rect();
    QRect iconRect;

    if (itemType() == Plugins) {
        iconRect.setWidth(itemRect.width());
        iconRect.setHeight(itemRect.height());
    } else {
        const int iconSize = std::min(itemRect.width(), itemRect.height()) * 0.8;
        iconRect.setWidth(iconSize);
        iconRect.setHeight(iconSize);
    }

    iconRect.moveTopLeft(itemRect.center() - iconRect.center());
    return iconRect;
}

void DockItem::showContextMenu()
{
    const QString menuJson = contextMenu();
    if (menuJson.isEmpty())
        return;

    QJsonDocument jsonDocument = QJsonDocument::fromJson(menuJson.toLocal8Bit().data());
    if (jsonDocument.isNull())
        return;

    QJsonObject jsonMenu = jsonDocument.object();

    qDeleteAll(m_contextMenu->actions());

    QJsonArray jsonMenuItems = jsonMenu.value("items").toArray();
    for (auto item : jsonMenuItems) {
        QJsonObject itemObj = item.toObject();
        QAction *action = new QAction(itemObj.value("itemText").toString());
        action->setCheckable(itemObj.value("isCheckable").toBool());
        action->setChecked(itemObj.value("checked").toBool());
        action->setData(itemObj.value("itemId").toString());
        action->setEnabled(itemObj.value("isActive").toBool());
        m_contextMenu->addAction(action);
    }

    hidePopup();
    emit requestWindowAutoHide(false);

    m_contextMenu->exec(QCursor::pos());

    onContextMenuAccepted();
}

void DockItem::menuActionClicked(QAction *action)
{
    invokedMenuItem(action->data().toString(), true);
}

void DockItem::onContextMenuAccepted()
{
    emit requestRefreshWindowVisible();
    emit requestWindowAutoHide(true);
}

void DockItem::showHoverTips()
{
    // another model popup window already exists
    if (PopupWindow->model())
        return;

    QWidget *const content = popupTips();
    if (!content)
        return;

    showPopupWindow(content);
}

void DockItem::showPopupWindow(QWidget *const content, const bool model)
{
    if (itemType() == App) {
        PopupWindow->setRadius(18);
    } else {
        PopupWindow->setRadius(6);
    }

    m_popupShown = true;
    m_lastPopupWidget = content;

    if (model)
        emit requestWindowAutoHide(false);

    DockPopupWindow *popup = PopupWindow.data();
    QWidget *lastContent = popup->getContent();
    if (lastContent)
        lastContent->setVisible(false);

    popup->resize(content->sizeHint());
    popup->setPosition(DockPosition);
    popup->setContent(content);

    const QPoint p = popupMarkPoint();
    if (!popup->isVisible())
        QMetaObject::invokeMethod(popup, "show", Qt::QueuedConnection, Q_ARG(QPoint, p), Q_ARG(bool, model));
    else
        popup->show(p, model);

    connect(popup, &DockPopupWindow::accept, this, &DockItem::popupWindowAccept, Qt::UniqueConnection);
}

void DockItem::popupWindowAccept()
{
    if (!PopupWindow->isVisible())
        return;

    disconnect(PopupWindow.data(), &DockPopupWindow::accept, this, &DockItem::popupWindowAccept);

    hidePopup();
}

void DockItem::showPopupApplet(QWidget *const applet)
{
    // another model popup window already exists
    if (PopupWindow->model())
        return;

    showPopupWindow(applet, true);
}

void DockItem::invokedMenuItem(const QString &itemId, const bool checked)
{
    Q_UNUSED(itemId)
    Q_UNUSED(checked)
}

const QString DockItem::contextMenu() const
{
    return QString();
}

QWidget *DockItem::popupTips()
{
    return nullptr;
}

/*!
 * \brief DockItem::checkAndResetTapHoldGestureState checks if a QTapAndHoldGesture
 * happens during the mouse press and release event pair.
 * \return true if yes, otherwise false.
 */
bool DockItem::checkAndResetTapHoldGestureState()
{
    bool ret = m_tapAndHold;
    m_tapAndHold = false;
    return ret;
}

const QPoint DockItem::popupMarkPoint()
{
    QPoint p = mapToGlobal(rect().topLeft());
    QPoint topP = topLevelWidget() ? topLevelWidget()->mapToGlobal(topLevelWidget()->rect().topLeft()) : mapToGlobal(rect().topLeft());
    const QRect r = rect();
    const QRect topR = topLevelWidget() ? topLevelWidget()->rect() : rect();

    QPoint popupPoint;
    switch (DockPosition) {
    case Top:
        popupPoint = QPoint(p.x() + r.width() / 2, topP.y() + topR.height() + POPUP_PADDING);
        break;
    case Bottom:
        popupPoint = QPoint(p.x() + r.width() / 2, topP.y() - POPUP_PADDING);
        break;
    case Left:
        popupPoint = QPoint(topP.x() + topR.width() + POPUP_PADDING, p.y() + r.height() / 2);
        break;
    case Right:
        popupPoint = QPoint(topP.x() - POPUP_PADDING, p.y() + r.height() / 2);
        break;
    }
    return popupPoint;
}

void DockItem::hidePopup()
{
    m_popupTipsDelayTimer->stop();
    m_popupAdjustDelayTimer->stop();
    m_popupShown = false;
    PopupWindow->hide();

    emit PopupWindow->accept();
    emit requestWindowAutoHide(true);
}

void DockItem::setDraging(bool bDrag)
{
    m_draging = bDrag;
}

void DockItem::hideNonModel()
{
    // auto hide if popup is not model window
    if (m_popupShown && !PopupWindow->model())
        hidePopup();
}

bool DockItem::isDragging()
{
    return m_draging;
}