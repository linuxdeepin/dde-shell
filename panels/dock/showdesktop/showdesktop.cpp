// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "applet.h"
#include "showdesktop.h"
#include "pluginfactory.h"
#include "../constants.h"

#include <QProcess>
#include <QGuiApplication>
#include <QDBusInterface>
#include <QLoggingCategory>
#include <DWindowManagerHelper>

Q_LOGGING_CATEGORY(showDesktop, "dde.shell.dock.showdesktop")

DGUI_USE_NAMESPACE

namespace dock {

ShowDesktop::ShowDesktop(QObject *parent)
    : DApplet(parent)
    , m_visible(true)
{
    connect(DWindowManagerHelper::instance(), &DWindowManagerHelper::hasCompositeChanged, this, &ShowDesktop::visibleChanged);
}

bool ShowDesktop::load()
{
    return QStringLiteral("xcb") == QGuiApplication::platformName();
}

bool ShowDesktop::init()
{
    DApplet::init();
    return true;
}

void ShowDesktop::toggleShowDesktop()
{
    QProcess::startDetached("/usr/lib/deepin-daemon/desktop-toggle", QStringList());
}

bool ShowDesktop::checkNeedShowDesktop()
{
    QDBusInterface wmInter("com.deepin.wm", "/com/deepin/wm", "com.deepin.wm");
    QList<QVariant> argumentList;
    QDBusMessage reply = wmInter.callWithArgumentList(QDBus::Block, QStringLiteral("GetIsShowDesktop"), argumentList);
    if (reply.type() == QDBusMessage::ReplyMessage && reply.arguments().count() == 1) {
        return !reply.arguments().at(0).toBool();
    }

    qCWarning(showDesktop) << "wm call GetIsShowDesktop fail, res:" << reply.type();
    return false;
}

DockItemInfo ShowDesktop::dockItemInfo()
{
    DockItemInfo info;
    info.name = "showdesktop";
    info.displayName = tr("Show Desktop");
    info.itemKey = "showdesktop";
    info.settingKey = "showdesktop";
    info.visible = visible();
    info.dccIcon = DCCIconPath + "showdesktop.svg";
    return info;
}

bool ShowDesktop::visible() const
{
    return m_visible && DWindowManagerHelper::instance()->hasComposite();
}

void ShowDesktop::setVisible(bool visible)
{
    if (m_visible != visible) {
        m_visible = visible;

        Q_EMIT visibleChanged();
    }
}

D_APPLET_CLASS(ShowDesktop)
}


#include "showdesktop.moc"
