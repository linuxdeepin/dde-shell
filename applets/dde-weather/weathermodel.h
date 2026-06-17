// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>

struct WeatherData {
    QString city;
    int temperature = 0;
    QString description;
    QString icon;
};

class WeatherModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        CityRole = Qt::UserRole + 1,
        TemperatureRole,
        DescriptionRole,
        IconRole,
    };

    explicit WeatherModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void updateData(const QList<WeatherData> &data);
    void clear();

private:
    QList<WeatherData> m_data;
};
