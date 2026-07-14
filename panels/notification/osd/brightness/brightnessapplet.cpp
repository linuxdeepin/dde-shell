// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "brightnessapplet.h"
#include "treelandbrightness.h"

#include "pluginfactory.h"

#include <QDBusConnection>
#include <QDBusReply>
#include <QGuiApplication>

#include <DDBusSender>
#include <DGuiApplicationHelper>

DGUI_USE_NAMESPACE

namespace osd {

static DDBusSender displayInter()
{
    return DDBusSender().service("org.deepin.dde.Display1")
        .path("/org/deepin/dde/Display1")
        .interface("org.deepin.dde.Display1");
}

BrightnessApplet::BrightnessApplet(QObject *parent)
    : DApplet(parent)
{

}

bool BrightnessApplet::load()
{
    m_isWayland = DGuiApplicationHelper::testAttribute(DGuiApplicationHelper::IsWaylandPlatform);
    if (!m_isWayland) {
        return DApplet::load();
    }

    // Display1 is unavailable on Wayland, so brightness is read from the
    // Treeland output-manager protocol. The provider caches the value the
    // compositor reports and updates it reactively; it never commits changes.
    m_treelandBrightness = new TreelandBrightness(this);
    connect(m_treelandBrightness, &TreelandBrightness::brightnessChanged, this, [this](double value) {
        setBrightness(value / 100.0);
        setIconName(fetchIconName());
    });
    connect(qApp, &QGuiApplication::primaryScreenChanged, m_treelandBrightness, [this]() {
        if (m_treelandBrightness) {
            m_treelandBrightness->refresh();
        }
    });
    m_treelandBrightness->refresh();
    return DApplet::load();
}

QString BrightnessApplet::iconName() const
{
    return m_iconName;
}

void BrightnessApplet::sync()
{
    if (m_isWayland) {
        if (m_treelandBrightness) {
            setBrightness(m_treelandBrightness->brightness() / 100.0);
        }
    } else {
        setBrightness(fetchBrightness());
    }
    setIconName(fetchIconName());
}

void BrightnessApplet::setIconName(const QString &newIconName)
{
    if (m_iconName == newIconName)
        return;
    m_iconName = newIconName;
    emit iconNameChanged();
}

QString BrightnessApplet::fetchIconName() const
{
    const QString iconName = "osd_brightness";
    if (m_brightness <= 0) {
        return iconName + "0";
    } else if (m_brightness <= 0.33) {
        return iconName + "33";
    } else if (m_brightness <= 0.66) {
        return iconName + "66";
    } else if (m_brightness <= 1) {
        return iconName + "100";
    }
    return "";
}

double BrightnessApplet::fetchBrightness() const
{
    QDBusReply<QVariant> brightness = displayInter().property("Brightness").get();
    if (!brightness.isValid()) {
        return 0;
    }

    QDBusReply<QVariant> primary = displayInter().property("Primary").get();
    if (!primary.isValid()) {
        return 0;
    }

    const auto brightnessInfo = qdbus_cast<QMap<QString, double>>(brightness.value());
    const auto primaryInfo = primary.value().toString();

    return brightnessInfo.value(primaryInfo);
}

double BrightnessApplet::brightness() const
{
    return m_brightness;
}

void BrightnessApplet::setBrightness(double newBrightness)
{
    if (qFuzzyCompare(m_brightness, newBrightness))
        return;
    m_brightness = newBrightness;
    emit brightnessChanged();
}

D_APPLET_CLASS(BrightnessApplet)

}

#include "brightnessapplet.moc"
