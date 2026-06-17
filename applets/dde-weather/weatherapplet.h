// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "applet.h"
#include "dsglobal.h"
#include "weathermodel.h"

DS_USE_NAMESPACE

class WeatherHelper;

class WeatherApplet : public DApplet
{
    Q_OBJECT
    Q_PROPERTY(WeatherModel *model READ model CONSTANT FINAL)
    Q_PROPERTY(QString currentCity READ currentCity NOTIFY currentCityChanged FINAL)
    Q_PROPERTY(int temperature READ temperature NOTIFY temperatureChanged FINAL)
    Q_PROPERTY(QString weatherDesc READ weatherDesc NOTIFY weatherDescChanged FINAL)
    Q_PROPERTY(QString weatherIcon READ weatherIcon NOTIFY weatherIconChanged FINAL)

public:
    explicit WeatherApplet(QObject *parent = nullptr);
    ~WeatherApplet();

    bool load() override;
    bool init() override;

    WeatherModel *model() const;
    QString currentCity() const;
    int temperature() const;
    QString weatherDesc() const;
    QString weatherIcon() const;

    Q_INVOKABLE void refresh();

Q_SIGNALS:
    void currentCityChanged();
    void temperatureChanged();
    void weatherDescChanged();
    void weatherIconChanged();

private:
    WeatherModel *m_model = nullptr;
    WeatherHelper *m_helper = nullptr;
    QString m_currentCity;
    int m_temperature = 0;
    QString m_weatherDesc;
    QString m_weatherIcon;
};
