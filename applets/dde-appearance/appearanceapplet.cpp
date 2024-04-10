// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "appearanceapplet.h"

#include "pluginfactory.h"

#include <QDBusError>
#include <QDebug>
#include <QDBusServiceWatcher>

DCORE_USE_NAMESPACE
DS_BEGIN_NAMESPACE
namespace dde {

AppearanceApplet::AppearanceApplet(QObject *parent)
    : DApplet(parent)
{
    auto watcher = new QDBusServiceWatcher(this);
    watcher->addWatchedService("org.deepin.dde.Appearance1");
    connect(watcher, &QDBusServiceWatcher::serviceRegistered, this, [this] (const QString & service) {
        initDBusProxy();
    });
}

AppearanceApplet::~AppearanceApplet()
{

}

bool AppearanceApplet::load()
{
    initDBusProxy();
    return DApplet::load();
}

qreal AppearanceApplet::opacity() const
{
    if (!m_interface)
        return -1;
    return m_interface->opacity();
}

void AppearanceApplet::initDBusProxy()
{
    qDebug() << "Init appearance dbus proxy.";
    m_interface.reset(new org::deepin::dde::Appearance1("org.deepin.dde.Appearance1",
                                                        "/org/deepin/dde/Appearance1",
                                                        QDBusConnection::sessionBus(),
                                                        this));
    if (!m_interface->isValid()) {
        m_interface.reset();
        qWarning() << "Failed to proxy Appearance, error:" << m_interface->lastError();
        return;
    }
    QObject::connect(m_interface.data(), &org::deepin::dde::Appearance1::OpacityChanged, this, &AppearanceApplet::opacityChanged);
}

D_APPLET_CLASS(AppearanceApplet)
}
DS_END_NAMESPACE

#include "appearanceapplet.moc"
