// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "qglobal.h"

#include "qgraphicslayout.h"
#include "quickproxywidget.h"
#include "quickproxywidget_p.h"
#include "private/qwidget_p.h"
#include "private/qapplication_p.h"

#include <QtCore/qdebug.h>
#include <QtGui/qevent.h>
#include <QtWidgets/qlayout.h>
#include <QtGui/qpainter.h>
#include <QtWidgets/qstyleoption.h>
#if QT_CONFIG(lineedit)
#include <QtWidgets/qlineedit.h>
#endif
#if QT_CONFIG(textedit)
#include <QtWidgets/qtextedit.h>
#endif

#include <QTimer>

QuickProxyWidgetPrivate::QuickProxyWidgetPrivate()
    : QQuickPaintedItemPrivate(),
    dragDropWidget(nullptr),
    // posChangeMode(NoMode),
    // sizeChangeMode(NoMode),
    visibleChangeMode(NoMode),
    enabledChangeMode(NoMode),
    styleChangeMode(NoMode),
    paletteChangeMode(NoMode),
    tooltipChangeMode(NoMode),
    focusFromWidgetToProxy(false),
    proxyIsGivingFocus(false)
{
}

QuickProxyWidgetPrivate::~QuickProxyWidgetPrivate()
{
}

void QuickProxyWidgetPrivate::init()
{
    Q_Q(QuickProxyWidget);

    q->setAcceptTouchEvents(true);
    q->setAcceptedMouseButtons(Qt::AllButtons);
    q->setFlag(QQuickItem::ItemAcceptsDrops, true);
    q->setAcceptHoverEvents(true);
}

void QuickProxyWidgetPrivate::sendWidgetMouseEvent(QHoverEvent *event)
{
    QMouseEvent mouseEvent(QEvent::MouseMove, event->position(), event->button(), event->buttons(), event->modifiers());
    sendWidgetMouseEvent(&mouseEvent);
    event->setAccepted(mouseEvent.isAccepted());
}

void QuickProxyWidgetPrivate::sendWidgetMouseEvent(QMouseEvent *event)
{
    if (!event || !widget || !widget->isVisible())
        return;

    Q_Q(QuickProxyWidget);

    // Find widget position and receiver.
    QPointF pos = event->pos();
    QPointer<QWidget> alienWidget = widget->childAt(pos.toPoint());
    QPointer<QWidget> receiver =  alienWidget ? alienWidget : widget;

    // if (QWidgetPrivate::nearestGraphicsProxyWidget(receiver) != q)
    //     return; //another proxywidget will handle the events

    // Translate QGraphicsSceneMouse events to QMouseEvents.
    QEvent::Type type = event->type();
    switch (event->type()) {
    case QEvent::MouseButtonPress:
        if (!embeddedMouseGrabber)
            embeddedMouseGrabber = receiver;
        else
            receiver = embeddedMouseGrabber;
        break;
    case QEvent::MouseButtonRelease:
        if (embeddedMouseGrabber)
            receiver = embeddedMouseGrabber;
        break;
    case QEvent::MouseButtonDblClick:
        if (!embeddedMouseGrabber)
            embeddedMouseGrabber = receiver;
        else
            receiver = embeddedMouseGrabber;
        break;
    case QEvent::MouseMove:
        if (embeddedMouseGrabber)
            receiver = embeddedMouseGrabber;
        break;
    default:
        Q_ASSERT_X(false, "QuickProxyWidget", "internal error");
        break;
    }

    if (!lastWidgetUnderMouse) {
        QApplicationPrivate::dispatchEnterLeave(embeddedMouseGrabber ? embeddedMouseGrabber : receiver, nullptr, event->screenPos());
        lastWidgetUnderMouse = receiver;
    }

    // Map event position from us to the receiver
    pos = mapToReceiver(pos, receiver);

    // Send mouse event.
    QMouseEvent mouseEvent(type, pos, receiver->mapTo(receiver->topLevelWidget(), pos.toPoint()),
                           receiver->mapToGlobal(pos.toPoint()),
                           event->button(), event->buttons(), event->modifiers(), Qt::MouseEventSynthesizedByApplication);

    QWidget *embeddedMouseGrabberPtr = (QWidget *)embeddedMouseGrabber.data();
    QApplicationPrivate::sendMouseEvent(receiver, &mouseEvent, alienWidget, widget,
                                        &embeddedMouseGrabberPtr, lastWidgetUnderMouse, event->spontaneous());
    embeddedMouseGrabber = embeddedMouseGrabberPtr;

    // Handle enter/leave events when last button is released from mouse
    // grabber child widget.
    if (embeddedMouseGrabber && type == QEvent::MouseButtonRelease && !event->buttons()) {
        Q_Q(QuickProxyWidget);
        QRect rect;
        rect.setRect(q->x(), q->y(), q->width(), q->height());
        if (rect.contains(event->pos()) && q->acceptHoverEvents())
            lastWidgetUnderMouse = alienWidget ? alienWidget : widget;
        else // released on the frame our outside the item, or doesn't accept hover events.
            lastWidgetUnderMouse = nullptr;

        QApplicationPrivate::dispatchEnterLeave(lastWidgetUnderMouse, embeddedMouseGrabber, event->screenPos());
        embeddedMouseGrabber = nullptr;

#ifndef QT_NO_CURSOR
        // ### Restore the cursor, don't override it.
        if (!lastWidgetUnderMouse)
            q->unsetCursor();
#endif
    }

    event->setAccepted(mouseEvent.isAccepted());
}

// void QuickProxyWidgetPrivate::updateProxyGeometryFromWidget()
// {
//     Q_Q(QuickProxyWidget);
//     if (!widget)
//         return;

//     QRectF widgetGeometry = widget->geometry();
//     // QWidget *parentWidget = widget->parentWidget();
//     // if (widget->isWindow()) {
//     //     QuickProxyWidget *proxyParent = nullptr;
//     //     if (parentWidget && (proxyParent = qobject_cast<QuickProxyWidget *>(q->parentWidget()))) {
//     //         // Nested window proxy (e.g., combobox popup), map widget to the
//     //         // parent widget's global coordinates, and map that to the parent
//     //         // proxy's child coordinates.
//     //         widgetGeometry.moveTo(proxyParent->subWidgetRect(parentWidget).topLeft()
//     //                               + parentWidget->mapFromGlobal(widget->pos()));
//     //     }
//     // }

//     // Adjust to size hint if the widget has never been resized.
//     if (!widget->size().isValid())
//         widgetGeometry.setSize(widget->sizeHint());

//     // Assign new geometry.
//     posChangeMode = QuickProxyWidgetPrivate::WidgetToProxyMode;
//     sizeChangeMode = QuickProxyWidgetPrivate::WidgetToProxyMode;
//     q->setGeometry(widgetGeometry);
//     posChangeMode = QuickProxyWidgetPrivate::NoMode;
//     sizeChangeMode = QuickProxyWidgetPrivate::NoMode;
// }


/*!
     \internal
*/
QPointF QuickProxyWidgetPrivate::mapToReceiver(const QPointF &pos, const QWidget *receiver) const
{
    QPointF p = pos;
    // Map event position from us to the receiver, preserving its
    // precision (don't use QWidget::mapFrom here).
    while (receiver && receiver != widget) {
        p -= QPointF(receiver->pos());
        receiver = receiver->parentWidget();
    }
    return p;
}

QuickProxyWidget::QuickProxyWidget(QQuickItem *parent)
    : QQuickPaintedItem(*new QuickProxyWidgetPrivate, parent)
{
    Q_D(QuickProxyWidget);
    d->init();
}

QuickProxyWidget::~QuickProxyWidget()
{
    Q_D(QuickProxyWidget);
    if (d->widget) {
        d->widget->removeEventFilter(this);
        // QObject::disconnect(d->widget, SIGNAL(destroyed()), this, SLOT(_q_removeWidgetSlot()));
    }
}

void QuickProxyWidget::setWidget(QWidget *widget)
{
    Q_D(QuickProxyWidget);
    d->setWidget_helper(widget, true);
}

void QuickProxyWidgetPrivate::setWidget_helper(QWidget *newWidget, bool autoShow)
{
    Q_Q(QuickProxyWidget);
    if (newWidget == widget)
        return;

    widget = newWidget;
    widget->setAttribute(Qt::WA_NoSystemBackground);
    widget->setAttribute(Qt::WA_TranslucentBackground);
    widget->createWinId();
    widget->windowHandle()->setMask(QRect(500, 500, 1, 1));
    qApp->installEventFilter(q);

    if (q->window()) {
        widget->windowHandle()->setParent(q->window());
        widget->setVisible(q->isVisible());
    }

    //     if (widget) {
    // QObject::disconnect(widget, SIGNAL(destroyed()), q, SLOT(_q_removeWidgetSlot()));
    //         widget->removeEventFilter(q);
    //         widget->setAttribute(Qt::WA_DontShowOnScreen, false);
    //         widget->d_func()->extra->proxyWidget = nullptr;
    //         resolveFont(inheritedFontResolveMask);
    //         resolvePalette(inheritedPaletteResolveMask);
    //         widget->update();

    //         const auto childItems = q->childItems();
    //         for (QGraphicsItem *child : childItems) {
    //             if (child->d_ptr->isProxyWidget()) {
    //                 QuickProxyWidget *childProxy = static_cast<QuickProxyWidget *>(child);
    //                 QWidget *parent = childProxy->widget();
    //                 while (parent && parent->parentWidget()) {
    //                     if (parent == widget)
    //                         break;
    //                     parent = parent->parentWidget();
    //                 }
    //                 if (!childProxy->widget() || parent != widget)
    //                     continue;
    //                 childProxy->setWidget(nullptr);
    //                 delete childProxy;
    //             }
    //         }

    //         widget = nullptr;
    // #ifndef QT_NO_CURSOR
    //         q->unsetCursor();
    // #endif
    //         q->setAcceptHoverEvents(false);
    //         if (!newWidget)
    //             q->update();
    //     }
    //     if (!newWidget)
    //         return;
    //     if (!newWidget->isWindow()) {
    //         const auto &extra = newWidget->parentWidget()->d_func()->extra;
    //         if (!extra || !extra->proxyWidget)  {
    //             qWarning("QuickProxyWidget::setWidget: cannot embed widget %p "
    //                      "which is not a toplevel widget, and is not a child of an embedded widget", newWidget);
    //             return;
    //         }
    //     }

    //     // Register this proxy within the widget's private.
    //     // ### This is a bit backdoorish
    //     QWExtra *extra = newWidget->d_func()->extra.get();
    //     if (!extra) {
    //         newWidget->d_func()->createExtra();
    //         extra = newWidget->d_func()->extra.get();
    //     }
    //     QuickProxyWidget **proxyWidget = &extra->proxyWidget;
    //     if (*proxyWidget) {
    //         if (*proxyWidget != q) {
    //             qWarning("QuickProxyWidget::setWidget: cannot embed widget %p"
    //                         "; already embedded", newWidget);
    //         }
    //         return;
    //     }
    //     *proxyWidget = q;

    //     newWidget->setAttribute(Qt::WA_DontShowOnScreen);
    //     newWidget->ensurePolished();
    //     // Do not wait for this widget to close before the app closes ###
    //     // shouldn't this widget inherit the attribute?
    //     newWidget->setAttribute(Qt::WA_QuitOnClose, false);
    //     q->setAcceptHoverEvents(true);

    //     if (newWidget->testAttribute(Qt::WA_NoSystemBackground))
    //         q->setAttribute(Qt::WA_NoSystemBackground);
    //     if (newWidget->testAttribute(Qt::WA_OpaquePaintEvent))
    //         q->setAttribute(Qt::WA_OpaquePaintEvent);

    //     widget = newWidget;

    //     // Changes only go from the widget to the proxy.
    //     enabledChangeMode = QuickProxyWidgetPrivate::WidgetToProxyMode;
    //     visibleChangeMode = QuickProxyWidgetPrivate::WidgetToProxyMode;
    //     posChangeMode = QuickProxyWidgetPrivate::WidgetToProxyMode;
    //     sizeChangeMode = QuickProxyWidgetPrivate::WidgetToProxyMode;

    // if ((autoShow && !newWidget->testAttribute(Qt::WA_WState_ExplicitShowHide)) || !newWidget->testAttribute(Qt::WA_WState_Hidden)) {
    // newWidget->show();
    // newWidget->setAttribute(Qt::WA_WState_Visible, true);
    // }

    // Copy the state from the widget onto the proxy.
#ifndef QT_NO_CURSOR
    if (newWidget->testAttribute(Qt::WA_SetCursor))
        q->setCursor(widget->cursor());
#endif
    q->setEnabled(newWidget->isEnabled());
    // q->setVisible(newWidget->isVisible());
    // q->setLayoutDirection(newWidget->layoutDirection());
    //     if (newWidget->testAttribute(Qt::WA_SetStyle))
    //         q->setStyle(widget->style());

    //     resolveFont(inheritedFontResolveMask);
    //     resolvePalette(inheritedPaletteResolveMask);

    //     if (!newWidget->testAttribute(Qt::WA_Resized))
    //         newWidget->adjustSize();

    //     q->setContentsMargins(newWidget->contentsMargins());
    //     q->setWindowTitle(newWidget->windowTitle());

    //     // size policies and constraints..
    //     q->setSizePolicy(newWidget->sizePolicy());
    //     QSize sz = newWidget->minimumSize();
    //     q->setMinimumSize(sz.isNull() ? QSizeF() : QSizeF(sz));
    //     sz = newWidget->maximumSize();
    //     q->setMaximumSize(sz.isNull() ? QSizeF() : QSizeF(sz));

    // updateProxyGeometryFromWidget();

    //     updateProxyInputMethodAcceptanceFromWidget();

    //     // Hook up the event filter to keep the state up to date.
    //     newWidget->installEventFilter(q);
    // QObject::connect(newWidget, SIGNAL(destroyed()), q, SLOT(_q_removeWidgetSlot()));

    // Changes no longer go only from the widget to the proxy.
    enabledChangeMode = QuickProxyWidgetPrivate::NoMode;
    visibleChangeMode = QuickProxyWidgetPrivate::NoMode;
    // posChangeMode = QuickProxyWidgetPrivate::NoMode;
    // sizeChangeMode = QuickProxyWidgetPrivate::NoMode;
}

QWidget *QuickProxyWidget::widget() const
{
    Q_D(const QuickProxyWidget);
    return d->widget;
}

// void QuickProxyWidget::setGeometry(const QRectF &rect)
// {
//     Q_D(QuickProxyWidget);
//     bool proxyResizesWidget = !d->posChangeMode && !d->sizeChangeMode;
//     if (proxyResizesWidget) {
//         d->posChangeMode = QuickProxyWidgetPrivate::ProxyToWidgetMode;
//         d->sizeChangeMode = QuickProxyWidgetPrivate::ProxyToWidgetMode;
//     }
//     if (d->widget) {
//         d->widget->setGeometry(rect.toRect());
//     }
//     if (proxyResizesWidget) {
//         d->posChangeMode = QuickProxyWidgetPrivate::NoMode;
//         d->sizeChangeMode = QuickProxyWidgetPrivate::NoMode;
//     }
// }

bool QuickProxyWidget::eventFilter(QObject *object, QEvent *event)
{
    Q_D(QuickProxyWidget);
    // update proxy widget geometry when tray window moved
    if (event->type() == QEvent::Move && object->property("__tray_window__").toBool()) {
        if (d->widget) {
            QRect rect;
            rect.setTopLeft(mapToGlobal(QPoint(0, 0)).toPoint());
            rect.setSize(d->widget->size());
            d->widget->setGeometry(rect);
        }
    }

    if (event->type() != QEvent::UpdateRequest && object != d->widget)
        return QQuickPaintedItem::eventFilter(object, event);

    if (object->isWidgetType()
        && d->widget->isAncestorOf(qobject_cast<QWidget *> (object))) {
        switch (event->type()) {
        case QEvent::UpdateRequest:
            update();
            break;
        case QEvent::Resize:
            // If the widget resizes itself, we resize the proxy too.
            // Prevent feed-back by checking the geometry change mode.
            // if (!d->sizeChangeMode)
            //     d->updateProxyGeometryFromWidget();
            break;
        case QEvent::Hide:
        case QEvent::Show:
            // If the widget toggles its visible state, the proxy will follow.
            if (!d->visibleChangeMode) {
                d->visibleChangeMode = QuickProxyWidgetPrivate::WidgetToProxyMode;
                setVisible(event->type() == QEvent::Show);
                d->visibleChangeMode = QuickProxyWidgetPrivate::NoMode;
            }
            break;
        case QEvent::EnabledChange:
            // If the widget toggles its enabled state, the proxy will follow.
            if (!d->enabledChangeMode) {
                d->enabledChangeMode = QuickProxyWidgetPrivate::WidgetToProxyMode;
                setEnabled(d->widget->isEnabled());
                d->enabledChangeMode = QuickProxyWidgetPrivate::NoMode;
            }
            break;
        case QEvent::StyleChange:
            // Propagate style changes to the proxy.
            if (!d->styleChangeMode) {
                d->styleChangeMode = QuickProxyWidgetPrivate::WidgetToProxyMode;
                // setStyle(d->widget->style());
                d->styleChangeMode = QuickProxyWidgetPrivate::NoMode;
            }
            break;
#ifndef QT_NO_TOOLTIP
        case QEvent::ToolTipChange:
            // Propagate tooltip change to the proxy.
            if (!d->tooltipChangeMode) {
                d->tooltipChangeMode = QuickProxyWidgetPrivate::WidgetToProxyMode;
                // setToolTip(d->widget->toolTip());
                d->tooltipChangeMode = QuickProxyWidgetPrivate::NoMode;
            }
            break;
#endif
        default:
            break;
        }
    }
    return QQuickPaintedItem::eventFilter(object, event);
}

#if QT_CONFIG(draganddrop)
void QuickProxyWidget::dragEnterEvent(QDragEnterEvent *event)
{
#if !QT_CONFIG(draganddrop)
    Q_UNUSED(event);
#else
    Q_D(QuickProxyWidget);
    if (!d->widget)
        return;

    QDragEnterEvent proxyDragEnter(event->position().toPoint(), event->dropAction(), event->mimeData(), event->buttons(), event->modifiers());
    proxyDragEnter.setAccepted(event->isAccepted());
    QCoreApplication::sendEvent(d->widget, &proxyDragEnter);
    event->setAccepted(proxyDragEnter.isAccepted());
    if (proxyDragEnter.isAccepted())    // we discard answerRect
        event->setDropAction(proxyDragEnter.dropAction());
#endif
}

void QuickProxyWidget::dragLeaveEvent(QDragLeaveEvent *event)
{
    Q_UNUSED(event);
#if QT_CONFIG(draganddrop)
    Q_D(QuickProxyWidget);
    if (!d->widget || !d->dragDropWidget)
        return;
    QDragLeaveEvent proxyDragLeave;
    QCoreApplication::sendEvent(d->dragDropWidget, &proxyDragLeave);
    d->dragDropWidget = nullptr;
#endif
}

void QuickProxyWidget::dragMoveEvent(QDragMoveEvent *event)
{
#if !QT_CONFIG(draganddrop)
    Q_UNUSED(event);
#else
    Q_D(QuickProxyWidget);
    if (!d->widget)
        return;
    QPointF p = event->position();
    event->ignore();
    QPointer<QWidget> subWidget = d->widget->childAt(p.toPoint());
    QPointer<QWidget> receiver =  subWidget ? subWidget : d->widget;
    bool eventDelivered = false;
    for (; receiver; receiver = receiver->parentWidget()) {
        if (!receiver->isEnabled() || !receiver->acceptDrops())
            continue;
        // Map event position from us to the receiver
        QPoint receiverPos = d->mapToReceiver(p, receiver).toPoint();
        if (receiver != d->dragDropWidget) {
            /* Qt will automatically send the leave event to last widget
             * that received the enter event, instead of the one we manually specified.
             * So send the leave event first.
            */
            if (d->dragDropWidget) {
                QDragLeaveEvent dragLeave;
                QCoreApplication::sendEvent(d->dragDropWidget, &dragLeave);
            }
            d->dragDropWidget = receiver;

            QDragEnterEvent dragEnter(receiverPos, event->possibleActions(), event->mimeData(), event->buttons(), event->modifiers());
            dragEnter.setDropAction(event->proposedAction());
            QCoreApplication::sendEvent(receiver, &dragEnter);
            event->setAccepted(dragEnter.isAccepted());
            event->setDropAction(dragEnter.dropAction());
            if (!event->isAccepted()) {
                // propagate to the parent widget
                continue;
            }

            d->lastDropAction = event->dropAction();
        }

        QDragMoveEvent dragMove(receiverPos, event->possibleActions(), event->mimeData(), event->buttons(), event->modifiers());
        event->setDropAction(d->lastDropAction);
        QCoreApplication::sendEvent(receiver, &dragMove);
        event->setAccepted(dragMove.isAccepted());
        event->setDropAction(dragMove.dropAction());
        if (event->isAccepted())
            d->lastDropAction = event->dropAction();
        eventDelivered = true;
        break;
    }

    if (!eventDelivered) {
        if (d->dragDropWidget) {
            // Leave the last drag drop item
            QDragLeaveEvent dragLeave;
            QCoreApplication::sendEvent(d->dragDropWidget, &dragLeave);
            d->dragDropWidget = nullptr;
        }
        // Propagate
        event->setDropAction(Qt::IgnoreAction);
    }
#endif
}

void QuickProxyWidget::dropEvent(QDropEvent *event)
{
#if !QT_CONFIG(draganddrop)
    Q_UNUSED(event);
#else
    Q_D(QuickProxyWidget);
    if (d->widget && d->dragDropWidget) {
        QPoint widgetPos = d->mapToReceiver(event->pos(), d->dragDropWidget).toPoint();
        QDropEvent dropEvent(widgetPos, event->possibleActions(), event->mimeData(), event->buttons(), event->modifiers());
        QCoreApplication::sendEvent(d->dragDropWidget, &dropEvent);
        event->setAccepted(dropEvent.isAccepted());
        d->dragDropWidget = nullptr;
    }
#endif
}
#endif

void QuickProxyWidget::hoverEnterEvent(QHoverEvent *event)
{
    Q_UNUSED(event);
}

void QuickProxyWidget::hoverLeaveEvent(QHoverEvent *event)
{
    Q_UNUSED(event);
    Q_D(QuickProxyWidget);
    // If hoverMove was compressed away, make sure we update properly here.
    if (d->lastWidgetUnderMouse) {
        QApplicationPrivate::dispatchEnterLeave(nullptr, d->lastWidgetUnderMouse, event->position());
        d->lastWidgetUnderMouse = nullptr;
    }
}

void QuickProxyWidget::hoverMoveEvent(QHoverEvent *event)
{
    Q_D(QuickProxyWidget);
#ifdef GRAPHICSPROXYWIDGET_DEBUG
    qDebug("QuickProxyWidget::hoverMoveEvent");
#endif

    // ignore repeat hovermove events
    if (event->position() == event->oldPosF())
        return;

    // Ignore events on the window frame.
    QRect rect;
    rect.setRect(x(), y(), width(), height());
    if (!d->widget || !rect.contains(event->pos())) {
        if (d->lastWidgetUnderMouse) {
            QApplicationPrivate::dispatchEnterLeave(nullptr, d->lastWidgetUnderMouse, event->position());
            d->lastWidgetUnderMouse = nullptr;
        }
        return;
    }

    d->embeddedMouseGrabber = nullptr;
    d->sendWidgetMouseEvent(event);
}

void QuickProxyWidget::mouseMoveEvent(QMouseEvent *event)
{
    Q_D(QuickProxyWidget);
#ifdef GRAPHICSPROXYWIDGET_DEBUG
    qDebug("QuickProxyWidget::mouseMoveEvent");
#endif
    d->sendWidgetMouseEvent(event);
}

void QuickProxyWidget::mousePressEvent(QMouseEvent *event)
{
    Q_D(QuickProxyWidget);
#ifdef GRAPHICSPROXYWIDGET_DEBUG
    qDebug("QuickProxyWidget::mousePressEvent");
#endif
    d->sendWidgetMouseEvent(event);
}

#if QT_CONFIG(wheelevent)
void QuickProxyWidget::wheelEvent(QWheelEvent *event)
{
    Q_D(QuickProxyWidget);
#ifdef GRAPHICSPROXYWIDGET_DEBUG
    qDebug("QuickProxyWidget::wheelEvent");
#endif
    if (!d->widget)
        return;

    QPointF pos = event->position();
    QPointer<QWidget> receiver = d->widget->childAt(pos.toPoint());
    if (!receiver)
        receiver = d->widget;

    // Map event position from us to the receiver
    pos = d->mapToReceiver(pos, receiver);

    // Send mouse event.
    // QPoint angleDelta;
    // if (event->orientation() == Qt::Horizontal)
    //     angleDelta.setX(event->delta());
    // else
    //     angleDelta.setY(event->delta());
    // pixelDelta, inverted, scrollPhase and source from the original QWheelEvent
    // were not preserved in the QGraphicsSceneWheelEvent unfortunately
    QWheelEvent wheelEvent(pos, event->position(), QPoint(), event->angleDelta(),
                           event->buttons(), event->modifiers(), Qt::NoScrollPhase, false);
    QPointer<QWidget> focusWidget = d->widget->focusWidget();
    // extern bool qt_sendSpontaneousEvent(QObject *, QEvent *);
    // qt_sendSpontaneousEvent(receiver, &wheelEvent);

    qApp->sendEvent(receiver, &wheelEvent);

    event->setAccepted(wheelEvent.isAccepted());

    // ### Remove, this should be done by proper focusIn/focusOut events.
    if (focusWidget && !focusWidget->hasFocus()) {
        focusWidget->update();
        focusWidget = d->widget->focusWidget();
        if (focusWidget && focusWidget->hasFocus())
            focusWidget->update();
    }
}
#endif

void QuickProxyWidget::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(QuickProxyWidget);
#ifdef GRAPHICSPROXYWIDGET_DEBUG
    qDebug("QuickProxyWidget::mouseReleaseEvent");
#endif
    d->sendWidgetMouseEvent(event);
}

void QuickProxyWidget::paint(QPainter *painter)
{
    Q_D(QuickProxyWidget);

    if (d->widget) {
        d->widget->render(painter);
    }
}

void QuickProxyWidget::itemChange(ItemChange change, const ItemChangeData &data)
{
    if (change == ItemSceneChange) {
        Q_D(QuickProxyWidget);
        if (d->widget) {
            if (data.window) {
                data.window->setProperty("__tray_window__", true);
                d->widget->windowHandle()->setParent(data.window);
                d->widget->setVisible(isVisible());
            } else {
                d->widget->hide();
                d->widget->windowHandle()->setParent(nullptr);
            }
        }
    } else if (change == ItemVisibleHasChanged) {
        Q_D(QuickProxyWidget);
        if (d->widget) {
            if (!this->window())
                return;
            auto window = d->widget->windowHandle();
            Q_ASSERT(window);
            window->setParent(this->window());
            window->setVisible(data.boolValue);
        }
    }

    update();
}

void QuickProxyWidget::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QuickProxyWidget);
    update();

    QQuickPaintedItem::geometryChange(newGeometry, oldGeometry);

    if (d->widget) {
        QRect rect;
        rect.setTopLeft(mapToGlobal(QPoint(0, 0)).toPoint());
        rect.setSize(d->widget->size());
        d->widget->setGeometry(rect);
    }
}

bool QuickProxyWidget::event(QEvent *event)
{
    update();

    return QQuickPaintedItem::event(event);
}

QT_END_NAMESPACE
