// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "keynotifyapplet.h"

#include "pluginfactory.h"
#include "treelandkeynotify.h"

#include <DDBusSender>
#include <DGuiApplicationHelper>

DCORE_USE_NAMESPACE

DS_BEGIN_NAMESPACE
namespace keynotify
{

KeyNotifyApplet::KeyNotifyApplet(QObject *parent)
    : DApplet(parent)
{
}

KeyNotifyApplet::~KeyNotifyApplet() = default;

bool KeyNotifyApplet::load()
{
    if (!Dtk::Gui::DGuiApplicationHelper::testAttribute(Dtk::Gui::DGuiApplicationHelper::IsWaylandPlatform)) {
        return DApplet::load();
    }

    m_keyNotify = new TreelandKeyNotify(this);
    connect(m_keyNotify, &TreelandKeyNotify::capsLockChanged, this, &KeyNotifyApplet::showCapsLockOsd);
    connect(m_keyNotify, &TreelandKeyNotify::numLockChanged, this, &KeyNotifyApplet::showNumLockOsd);
    initConfig();
    m_keyNotify->setCapsLockEnabled(m_config->value(QStringLiteral("capslockToggle")).toBool());
    return DApplet::load();
}

void KeyNotifyApplet::showCapsLockOsd(bool locked)
{
    sendOsd(locked ? QStringLiteral("CapsLockOn") : QStringLiteral("CapsLockOff"));
}

void KeyNotifyApplet::showNumLockOsd(bool locked)
{
    sendOsd(locked ? QStringLiteral("NumLockOn") : QStringLiteral("NumLockOff"));
}

void KeyNotifyApplet::updateCapsLockToggle(const QString &key)
{
    if (key != QStringLiteral("capslockToggle")) {
        return;
    }

    m_keyNotify->setCapsLockEnabled(m_config->value(key).toBool());
}

void KeyNotifyApplet::sendOsd(const QString &osdType)
{
    DDBusSender().service("org.deepin.dde.shell").path("/org/deepin/dde/shell/osd").interface("org.deepin.dde.shell.osd").method("ShowOSD").arg(osdType).call();
}

void KeyNotifyApplet::initConfig()
{
    m_config = Dtk::Core::DConfig::create(QStringLiteral("org.deepin.dde.daemon"), QStringLiteral("org.deepin.dde.daemon.keyboard"), QString(), this);
    connect(m_config, &Dtk::Core::DConfig::valueChanged, this, &KeyNotifyApplet::updateCapsLockToggle);
}

D_APPLET_CLASS(KeyNotifyApplet)

}
DS_END_NAMESPACE

#include "keynotifyapplet.moc"
