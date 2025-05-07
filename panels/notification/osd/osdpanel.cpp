// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "osdpanel.h"

#include "osddbusadaptor.h"
#include "pluginfactory.h"
#include <QDBusConnectionInterface>

#include <QDBusConnection>
#include <QTimer>
#include <QLoggingCategory>
#include <QDBusConnection>
#include <QDBusError>

namespace osd {

OsdPanel::OsdPanel(QObject *parent)
    : DPanel(parent)
{
}

bool OsdPanel::load()
{
    return DPanel::load();
}

bool OsdPanel::init()
{
    auto bus = QDBusConnection::sessionBus();
    if (!bus.registerObject(QStringLiteral("/org/deepin/dde/shell/osd"),
                       QStringLiteral("org.deepin.dde.shell.osd"),
                       this,
                           QDBusConnection::ExportAllSlots)) {
        return false;
    }

    bus.interface()->registerService("org.deepin.dde.Osd1",
                                            QDBusConnectionInterface::ReplaceExistingService,
                                            QDBusConnectionInterface::AllowReplacement);
    if (!bus.registerObject("/", "org.deepin.dde.Osd1", this, QDBusConnection::ExportAllSlots)) {
        return false;
    }
    new OsdDBusAdaptor(this);

    m_osdTimer = new QTimer(this);
    m_osdTimer->setInterval(m_interval);
    m_osdTimer->setSingleShot(true);
    QObject::connect(m_osdTimer, &QTimer::timeout, this, &OsdPanel::hideOsd);
    return DPanel::init();
}

Q_LOGGING_CATEGORY(osdLog, "dde.shell.osd")

QString OsdPanel::osdType() const
{
    return m_osdType;
}

bool OsdPanel::visible() const
{
    return m_visible;
}

void OsdPanel::ShowOSD(const QString &text)
{
    qCInfo(osdLog()) << "show text" << text;
    m_osdTimer->setInterval(text == "SwitchWM3D" ? 2000 : 1000);

    QTimer::singleShot(100, this, [this, text]() {
        setOsdType(text);
        showOsd();
    });
}

void OsdPanel::hideOsd()
{
    m_osdTimer->stop();
    setVisible(false);
    updateLastOsdType({});
}

void OsdPanel::showOsd()
{
    m_osdTimer->stop();

    m_osdTimer->start();
    setVisible(true);
    updateLastOsdType(m_osdType);
}

void OsdPanel::setVisible(const bool visible)
{
    if (visible == m_visible)
        return;
    m_visible = visible;
    Q_EMIT visibleChanged();
}

void OsdPanel::setOsdType(const QString &osdType)
{
    m_osdType = osdType;
    emit osdTypeChanged(m_osdType);
}

void OsdPanel::updateLastOsdType(const QString &osdType)
{
    if (m_lastOsdType != osdType) {
        m_lastOsdType = osdType;
    }
}

QString OsdPanel::lastOsdType() const
{
    return m_lastOsdType;
}

D_APPLET_CLASS(OsdPanel)

}

#include "osdpanel.moc"
