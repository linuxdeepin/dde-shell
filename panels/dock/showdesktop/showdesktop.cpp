// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "showdesktop.h"
#include "applet.h"
#include "pluginfactory.h"
#include "treelandwindowmanager.h"

#include <QProcess>
#include <QGuiApplication>
#include <QDBusInterface>
#include <QLoggingCategory>

#include <DConfig>

DCORE_USE_NAMESPACE

Q_LOGGING_CATEGORY(showDesktop, "org.deepin.dde.shell.dock.showdesktop")

namespace dock {

ShowDesktop::ShowDesktop(QObject *parent)
    : DApplet(parent)
    , m_windowManager(nullptr)
    , m_dockConfig(nullptr)
    , m_visible(true)
{
    // 创建DConfig实例来读取dock配置
    m_dockConfig = DConfig::create("org.deepin.dde.shell", "org.deepin.ds.dock", QString(), this);
    
    if (m_dockConfig) {
        // 监听配置变化
        connect(m_dockConfig, &DConfig::valueChanged, this, [this](const QString &key) {
            if (key == "enableShowDesktop") {
                onEnableShowDesktopChanged();
            }
        });
    }
}

bool ShowDesktop::load()
{
    return true;
}

bool ShowDesktop::init()
{
    if (QStringLiteral("wayland") == QGuiApplication::platformName()) {
        m_windowManager = new TreelandWindowManager(this);
    }
    
    // 从配置中读取初始的可见性状态
    if (m_dockConfig && m_dockConfig->isValid()) {
        m_visible = m_dockConfig->value("enableShowDesktop", true).toBool();
    }
    
    DApplet::init();
    return true;
}

void ShowDesktop::toggleShowDesktop()
{
    if (m_windowManager) {
        m_windowManager->desktopToggle();
        return;
    }

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

bool ShowDesktop::visible() const
{
    return m_visible;
}

void ShowDesktop::setVisible(bool visible)
{
    if (m_visible != visible) {
        m_visible = visible;
        Q_EMIT visibleChanged();
    }
}

void ShowDesktop::onEnableShowDesktopChanged()
{
    if (m_dockConfig && m_dockConfig->isValid()) {
        bool enabled = m_dockConfig->value("enableShowDesktop", true).toBool();
        qCDebug(showDesktop) << "onEnableShowDesktopChanged" << enabled;
        setVisible(enabled);
    }
}

D_APPLET_CLASS(ShowDesktop)
}


#include "showdesktop.moc"
