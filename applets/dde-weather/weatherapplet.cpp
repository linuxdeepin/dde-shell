// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "weatherapplet.h"
#include "weathermodel.h"
#include "weatherhelper.h"
#include "pluginfactory.h"

#include <DConfig>

WeatherApplet::WeatherApplet(QObject *parent)
    : DApplet(parent)
    , m_model(new WeatherModel(this))
    , m_helper(new WeatherHelper(this))
{
    connect(m_helper, &WeatherHelper::weatherUpdated, this, [this](const QString &city, int temp, const QString &desc, const QString &icon) {
        if (m_currentCity != city) {
            m_currentCity = city;
            Q_EMIT currentCityChanged();
        }
        if (m_temperature != temp) {
            m_temperature = temp;
            Q_EMIT temperatureChanged();
        }
        if (m_weatherDesc != desc) {
            m_weatherDesc = desc;
            Q_EMIT weatherDescChanged();
        }
        if (m_weatherIcon != icon) {
            m_weatherIcon = icon;
            Q_EMIT weatherIconChanged();
        }
    });

    connect(m_helper, &WeatherHelper::forecastUpdated, m_model, &WeatherModel::updateData);
}

WeatherApplet::~WeatherApplet()
{
}

bool WeatherApplet::load()
{
    DCORE_USE_NAMESPACE;
    auto config = DConfig::create("org.deepin.dde.shell", "org.deepin.ds.weather");
    if (config) {
        m_helper->setCity(config->value("city", "auto").toString());
        m_helper->setRefreshInterval(config->value("refreshInterval", 30).toInt());
        delete config;
    }
    return true;
}

bool WeatherApplet::init()
{
    DApplet::init();
    m_helper->refresh();
    return true;
}

WeatherModel *WeatherApplet::model() const
{
    return m_model;
}

QString WeatherApplet::currentCity() const
{
    return m_currentCity;
}

int WeatherApplet::temperature() const
{
    return m_temperature;
}

QString WeatherApplet::weatherDesc() const
{
    return m_weatherDesc;
}

QString WeatherApplet::weatherIcon() const
{
    return m_weatherIcon;
}

void WeatherApplet::refresh()
{
    m_helper->refresh();
}

D_APPLET_CLASS(WeatherApplet)

#include "weatherapplet.moc"
