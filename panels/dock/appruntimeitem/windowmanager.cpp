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
    return m_windowList.size();
}

QVariant WindowManager::data(const QModelIndex &index, int role) const {
    if (index.row() < 0 || index.row() >= m_windowList.size()) {
        return QVariant();
    }

    const WindowInfo_1 &info = m_windowList[index.row()];
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

void WindowManager::setWindowInfo_qiantai(const QString &name, uint id) {
    bool found = false;
    for (const auto &info : m_windowList) {
        if (info.name == name) {
            found = true;
            break;
        }
    }
    if (!found) {
        WindowInfo_1 info;
        info.name = "前台应用："+name;
        info.id = id;
        info.startTime = QDateTime::currentDateTime();
        beginInsertRows(QModelIndex(), m_windowList.size(), m_windowList.size());
        m_windowList.append(info);
        endInsertRows();
    }

}

void WindowManager::setWindowInfo_houtai(const QString &name, uint id)
{
    bool found = false;
    for (const auto &info : m_windowList) {
        if (info.name == name) {
            found = true;
            break;
        }
    }
    if (!found) {
        WindowInfo_1 info;
        info.name = "后台应用："+name;
        info.id = id;
        info.startTime = QDateTime::currentDateTime();
        beginInsertRows(QModelIndex(), m_windowList.size(), m_windowList.size());
        m_windowList.append(info);
        endInsertRows();
    }
}

void WindowManager::WindowDetroyInfo(uint id)
{
    // 查找要移除的项，只根据 id 进行匹配
    for (int i = 0; i < m_windowList.size(); ++i) {
        if (m_windowList[i].id == id) {
            // 开始移除行
            beginRemoveRows(QModelIndex(), i, i);
            m_windowList.removeAt(i);
            endRemoveRows();
            return;
        }
    }
}
