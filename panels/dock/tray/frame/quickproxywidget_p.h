// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef QUICKPROXYWIDGET_P_H
#define QUICKPROXYWIDGET_P_H

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include "quickproxywidget.h"
#include "private/qobject_p.h"
#include "private/qquickpainteditem_p.h"

QT_REQUIRE_CONFIG(graphicsview);

QT_BEGIN_NAMESPACE

class Q_AUTOTEST_EXPORT QuickProxyWidgetPrivate : public QQuickPaintedItemPrivate
{
    Q_DECLARE_PUBLIC(QuickProxyWidget)
public:
    QuickProxyWidgetPrivate();
    ~QuickProxyWidgetPrivate();

    void init();
    void sendWidgetMouseEvent(QMouseEvent *event);
    void sendWidgetMouseEvent(QHoverEvent *event);
    void setWidget_helper(QWidget *widget, bool autoShow);

    QPointer<QWidget> widget;
    QPointer<QWidget> lastWidgetUnderMouse;
    QPointer<QWidget> embeddedMouseGrabber;
    QWidget *dragDropWidget;
    Qt::DropAction lastDropAction;

    // void updateProxyGeometryFromWidget();

    QPointF mapToReceiver(const QPointF &pos, const QWidget *receiver) const;

    enum ChangeMode {
        NoMode,
        ProxyToWidgetMode,
        WidgetToProxyMode
    };
    // quint32 posChangeMode : 2;
    // quint32 sizeChangeMode : 2;
    quint32 visibleChangeMode : 2;
    quint32 enabledChangeMode : 2;
    quint32 styleChangeMode : 2;
    quint32 paletteChangeMode : 2;
    quint32 tooltipChangeMode : 2;
    quint32 focusFromWidgetToProxy : 1;
    quint32 proxyIsGivingFocus : 1;
};

QT_END_NAMESPACE

#endif
