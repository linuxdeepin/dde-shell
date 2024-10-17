// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "appruntimeitem.h"
#include "applet.h"
#include "pluginfactory.h"

#include "../constants.h"

#include <DDBusSender>
#include <DDciIcon>
#include <DGuiApplicationHelper>

#include <QGuiApplication>
#include <QBuffer>
#include<QQuickView>
#include<QQmlContext>
#include <QtQml>

DGUI_USE_NAMESPACE

namespace dock {
AppRuntimeItem::AppRuntimeItem(QObject *parent)
    : DApplet(parent)
    , m_Visible(true)
    , m_appRuntimeVisible(false){}

void AppRuntimeItem::toggleruntimeitem()
{
    qmlRegisterType<WindowManager>("WindowModule", 1, 0, "WindowManager");
    qmlRegisterType<XcbGetInfo>("XcbGetModule", 1, 0, "XcbGetInfo");
}

DockItemInfo AppRuntimeItem::dockItemInfo()
{
    DockItemInfo info;
    info.name = "appruntime";
    info.displayName = tr("App_runtime");
    info.itemKey = "appruntime";
    info.settingKey = "appruntime";
    info.visible = m_Visible;
    info.dccIcon = DCCIconPath + "appruntime.svg";
    return info;
}
void AppRuntimeItem::setVisible(bool visible)
{
    if (m_Visible != visible) {
        m_Visible = visible;

        Q_EMIT visibleChanged(visible);
    }
}

void AppRuntimeItem::onappruntimeVisibleChanged(bool visible)
{
    if (m_appRuntimeVisible != visible) {
        m_appRuntimeVisible = visible;
        Q_EMIT appruntimeVisibleChanged(visible);
    }
}

D_APPLET_CLASS(AppRuntimeItem)
}

#include "appruntimeitem.moc"
