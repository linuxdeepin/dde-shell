// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "weatherhelper.h"

#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>

static const char *WEATHER_API_URL = "https://api.openweathermap.org/data/2.5/weather";
static const char *FORECAST_API_URL = "https://api.openweathermap.org/data/2.5/forecast";

WeatherHelper::WeatherHelper(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
    , m_timer(new QTimer(this))
{
    connect(m_nam, &QNetworkAccessManager::finished, this, &WeatherHelper::onReplyFinished);
    connect(m_timer, &QTimer::timeout, this, &WeatherHelper::refresh);
}

void WeatherHelper::setCity(const QString &city)
{
    m_city = city;
}

void WeatherHelper::setRefreshInterval(int minutes)
{
    m_refreshInterval = minutes;
    m_timer->stop();
    if (minutes > 0) {
        m_timer->start(minutes * 60 * 1000);
    }
}

void WeatherHelper::refresh()
{
    if (m_city == "auto" || m_city.isEmpty()) {
        updateWithMockData();
        return;
    }

    QUrl url(WEATHER_API_URL);
    QUrlQuery query;
    query.addQueryItem("q", m_city);
    query.addQueryItem("appid", "demo");
    query.addQueryItem("units", "metric");
    query.addQueryItem("lang", "zh_cn");
    url.setQuery(query);

    QNetworkRequest request(url);
    m_nam->get(request);
}

void WeatherHelper::onReplyFinished(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        updateWithMockData();
        reply->deleteLater();
        return;
    }

    parseWeatherData(reply->readAll());
    reply->deleteLater();
}

void WeatherHelper::parseWeatherData(const QByteArray &data)
{
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject())
        return;

    QJsonObject obj = doc.object();

    QString city = obj.value("name").toString();
    QJsonObject main = obj.value("main").toObject();
    int temp = static_cast<int>(main.value("temp").toDouble());

    QJsonArray weatherArray = obj.value("weather").toArray();
    QString desc;
    QString icon;
    if (!weatherArray.isEmpty()) {
        QJsonObject weather = weatherArray.first().toObject();
        desc = weather.value("description").toString();
        icon = weatherCodeToIcon(weather.value("icon").toString());
    }

    Q_EMIT weatherUpdated(city, temp, desc, icon);
}

void WeatherHelper::updateWithMockData()
{
    Q_EMIT weatherUpdated(tr("Demo City"), 26, tr("Partly cloudy"), "⛅");

    QList<WeatherData> forecast;
    forecast.append({ tr("Demo City"), 26, tr("Partly cloudy"), "⛅" });
    forecast.append({ tr("Demo City"), 24, tr("Light rain"), "🌧" });
    forecast.append({ tr("Demo City"), 28, tr("Sunny"), "☀" });
    forecast.append({ tr("Demo City"), 22, tr("Overcast"), "☁" });
    forecast.append({ tr("Demo City"), 25, tr("Clear"), "🌙" });
    Q_EMIT forecastUpdated(forecast);
}

QString WeatherHelper::weatherCodeToIcon(const QString &code) const
{
    if (code.startsWith("01")) return "☀";
    if (code.startsWith("02")) return "⛅";
    if (code.startsWith("03") || code.startsWith("04")) return "☁";
    if (code.startsWith("09") || code.startsWith("10")) return "🌧";
    if (code.startsWith("11")) return "⛈";
    if (code.startsWith("13")) return "❄";
    if (code.startsWith("50")) return "🌫";
    return "☀";
}
