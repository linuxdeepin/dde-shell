// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "weathermodel.h"

#include <QObject>
#include <QTimer>
#include <QNetworkAccessManager>

class WeatherHelper : public QObject
{
    Q_OBJECT

public:
    explicit WeatherHelper(QObject *parent = nullptr);

    void setCity(const QString &city);
    void setRefreshInterval(int minutes);
    void refresh();

Q_SIGNALS:
    void weatherUpdated(const QString &city, int temperature, const QString &description, const QString &icon);
    void forecastUpdated(const QList<WeatherData> &forecast);

private Q_SLOTS:
    void onReplyFinished(QNetworkReply *reply);

private:
    void parseWeatherData(const QByteArray &data);
    void updateWithMockData();
    QString weatherCodeToIcon(const QString &code) const;

    QNetworkAccessManager *m_nam = nullptr;
    QTimer *m_timer = nullptr;
    QString m_city = "auto";
    int m_refreshInterval = 30;
};
