// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "appearanceapplet.h"

#include "pluginfactory.h"

#include <QDBusError>
#include <QDBusMessage>
#include <QDBusReply>
#include <QDBusVariant>
#include <QDebug>
#include <QDBusServiceWatcher>

DCORE_USE_NAMESPACE
DS_BEGIN_NAMESPACE
namespace dde {

namespace {
bool isOpacityChangeType(const QString &type)
{
    return type.compare(QStringLiteral("opacity"), Qt::CaseInsensitive) == 0
        || type.compare(QStringLiteral("windowopacity"), Qt::CaseInsensitive) == 0;
}
}

AppearanceApplet::AppearanceApplet(QObject *parent)
    : DApplet(parent)
{
    auto watcher = new QDBusServiceWatcher(this);
    watcher->addWatchedService("org.deepin.dde.Appearance1");
    watcher->setConnection(QDBusConnection::sessionBus());
    connect(watcher, &QDBusServiceWatcher::serviceRegistered, this, [this] (const QString & service) {
        Q_UNUSED(service)
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
    if (m_opacity < 0)
        return -1;

    // The minimum opacity is 0.2
    return std::max(0.2, m_opacity);
}

void AppearanceApplet::initDBusProxy()
{
    qDebug() << "Init appearance dbus proxy.";
    m_interface.reset(new org::deepin::dde::Appearance1("org.deepin.dde.Appearance1",
                                                        "/org/deepin/dde/Appearance1",
                                                        QDBusConnection::sessionBus(),
                                                        this));
    if (!m_interface->isValid()) {
        qWarning() << "Failed to proxy Appearance, error:" << m_interface->lastError();
        m_interface.reset();
        return;
    }

    QObject::connect(m_interface.data(), &org::deepin::dde::Appearance1::Changed, this,
                     [this](const QString &type, const QString &) {
        if (isOpacityChangeType(type)) {
            refreshOpacity();
            Q_EMIT opacityChanged();
        }
    });
    QObject::connect(m_interface.data(), &org::deepin::dde::Appearance1::Refreshed, this,
                     [this](const QString &type) {
        if (isOpacityChangeType(type)) {
            refreshOpacity();
            Q_EMIT opacityChanged();
        }
    });
    refreshOpacity();
    Q_EMIT opacityChanged();
}

void AppearanceApplet::refreshOpacity()
{
    QDBusMessage message = QDBusMessage::createMethodCall(QStringLiteral("org.deepin.dde.Appearance1"),
                                                          QStringLiteral("/org/deepin/dde/Appearance1"),
                                                          QStringLiteral("org.freedesktop.DBus.Properties"),
                                                          QStringLiteral("Get"));
    message << QStringLiteral("org.deepin.dde.Appearance1") << QStringLiteral("Opacity");

    QDBusReply<QDBusVariant> reply = QDBusConnection::sessionBus().call(message);
    if (!reply.isValid()) {
        qWarning() << "Failed to get Appearance opacity, error:" << reply.error();
        m_opacity = -1;
        return;
    }

    m_opacity = reply.value().variant().toReal();
}

D_APPLET_CLASS(AppearanceApplet)
}
DS_END_NAMESPACE

#include "appearanceapplet.moc"
