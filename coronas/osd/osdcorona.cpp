// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "osdcorona.h"

#include "pluginfactory.h"

#include <QDBusConnection>
#include <QTimer>
#include <QLoggingCategory>
#include <QDBusConnection>
#include <QDBusError>

DS_BEGIN_NAMESPACE
namespace osd {

OsdCorona::OsdCorona(QObject *parent)
    : DCorona(parent)
{
}

void OsdCorona::load()
{
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.registerService("org.deepin.dde.Shell")) {
        qWarning() << "register failed" << bus.lastError().message();
    }

    DCorona::load();
}

void OsdCorona::init()
{
    auto bus = QDBusConnection::sessionBus();
    bus.registerObject(QStringLiteral("/org/deepin/osdService"),
                       this,
                       QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllSignals);

    m_osdTimer = new QTimer(this);
    m_osdTimer->setInterval(m_interval);
    m_osdTimer->setSingleShot(true);
    QObject::connect(m_osdTimer, &QTimer::timeout, this, &OsdCorona::hideOsd);
    DCorona::init();
}

Q_LOGGING_CATEGORY(osdLog, "dde.shell.osd")

QString OsdCorona::osdType() const
{
    return m_osdType;
}

bool OsdCorona::visible() const
{
    return m_visible;
}

void OsdCorona::showText(const QString &text)
{
    qCInfo(osdLog()) << "show text" << text;
    m_osdTimer->setInterval(text == "SwitchWM3D" ? 2000 : 1000);

    setOsdType(text);
    showOsd();
}

void OsdCorona::hideOsd()
{
    m_osdTimer->stop();
    setVisible(false);
}

void OsdCorona::showOsd()
{
    m_osdTimer->stop();

    m_osdTimer->start();
    setVisible(true);
}

void OsdCorona::setVisible(const bool visible)
{
    if (visible == m_visible)
        return;
    m_visible = visible;
    Q_EMIT visibleChanged();
}

void OsdCorona::setOsdType(const QString &osdType)
{
    m_osdType = osdType;
    emit osdTypeChanged(m_osdType);
}

D_APPLET_CLASS(OsdCorona)

}
DS_END_NAMESPACE

#include "osdcorona.moc"
