// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "weathermodel.h"

WeatherModel::WeatherModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int WeatherModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_data.count();
}

QVariant WeatherModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_data.count())
        return {};

    const auto &item = m_data.at(index.row());
    switch (role) {
    case CityRole:
        return item.city;
    case TemperatureRole:
        return item.temperature;
    case DescriptionRole:
        return item.description;
    case IconRole:
        return item.icon;
    default:
        return {};
    }
}

QHash<int, QByteArray> WeatherModel::roleNames() const
{
    return {
        { CityRole, "city" },
        { TemperatureRole, "temperature" },
        { DescriptionRole, "description" },
        { IconRole, "icon" },
    };
}

void WeatherModel::updateData(const QList<WeatherData> &data)
{
    beginResetModel();
    m_data = data;
    endResetModel();
}

void WeatherModel::clear()
{
    beginResetModel();
    m_data.clear();
    endResetModel();
}
