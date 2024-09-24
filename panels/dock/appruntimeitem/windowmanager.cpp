// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "windowmanager.h"
#include <QDebug>

WindowManager::WindowManager(QObject *parent)
    : QAbstractListModel(parent)
{
}

int WindowManager::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent)
    return mWindowList.size();
}

QVariant WindowManager::data(const QModelIndex &index, int role) const {
    if (index.row() < 0 || index.row() >= mWindowList.size()) {
        return QVariant();
    }

    const AppRuntimeInfo &info = mWindowList[index.row()];
    switch (role) {
    case NameRole:
        return info.name;
    case IdRole:
        return info.id;
    case StartTimeRole:
        return info.startTime.toMSecsSinceEpoch();
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> WindowManager::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[IdRole] = "id";
    roles[StartTimeRole] = "startTime";
    return roles;
}

void WindowManager::setWindowInfoForeground(const QString &name, uint id) {
    bool found = false;
    for (const auto &info : mWindowList) {
        if (info.name == name) {
            found = true;
            break;
        }
    }
    if (!found) {
        AppRuntimeInfo info;
        info.name = "ForegroundApp："+name;
        info.id = id;
        info.startTime = QDateTime::currentDateTime();
        beginInsertRows(QModelIndex(), mWindowList.size(), mWindowList.size());
        mWindowList.append(info);
        endInsertRows();
    }

}

void WindowManager::setWindowInfoBackground(const QString &name, uint id)
{
    bool found = false;
    for (const auto &info : mWindowList) {
        if (info.name == name) {
            found = true;
            break;
        }
    }
    if (!found) {
        AppRuntimeInfo info;
        info.name = "BackgroundApp："+name;
        info.id = id;
        info.startTime = QDateTime::currentDateTime();
        beginInsertRows(QModelIndex(), mWindowList.size(), mWindowList.size());
        mWindowList.append(info);
        endInsertRows();
    }
}

void WindowManager::WindowDetroyInfo(uint id)
{
    // Find the item to remove, matching only by id
    for (int i = 0; i < mWindowList.size(); ++i) {
        if (mWindowList[i].id == id) {
            // Start removing the row
            beginRemoveRows(QModelIndex(), i, i);
            mWindowList.removeAt(i);
            endRemoveRows();
            return;
        }
    }
}
