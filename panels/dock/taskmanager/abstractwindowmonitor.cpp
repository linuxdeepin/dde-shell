// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "abstractwindowmonitor.h"
#include "abstractwindow.h"
#include "globals.h"
#include "taskmanager.h"

namespace dock {
AbstractWindowMonitor::AbstractWindowMonitor(QObject* parent)
    : QAbstractListModel(parent)
{
}

QHash<int, QByteArray> AbstractWindowMonitor::roleNames() const
{
    return {{TaskManager::WinIdRole, MODEL_WINID},
            {TaskManager::PidRole, MODEL_PID},
            {TaskManager::IdentityRole, MODEL_IDENTIFY},
            {TaskManager::WinIconRole, MODEL_WINICON},
            {TaskManager::WinTitleRole, MODEL_TITLE},
            {TaskManager::ActiveRole, MODEL_ACTIVE},
            {TaskManager::ShouldSkipRole, MODEL_SHOULDSKIP}};
}

int AbstractWindowMonitor::rowCount(const QModelIndex &parent) const
{
    return m_trackedWindows.size();
}

QVariant AbstractWindowMonitor::data(const QModelIndex &index, int role) const
{
    auto pos = index.row();
    if (pos >= m_trackedWindows.size())
        return QVariant();
    auto window = m_trackedWindows[pos];

    switch (role) {
    case TaskManager::WinIdRole:
        return window->id();
    case TaskManager::PidRole:
        return window->pid();
    case TaskManager::IdentityRole:
        return window->identity();
    case TaskManager::WinIconRole:
        return window->icon();
    case TaskManager::WinTitleRole:
        return window->title();
    case TaskManager::ActiveRole:
        return window->isActive();
    case TaskManager::ShouldSkipRole:
        return window->shouldSkip();
    }

    return QVariant();
}

void AbstractWindowMonitor::trackWindow(AbstractWindow* window)
{
    beginInsertRows(QModelIndex(), m_trackedWindows.size(), m_trackedWindows.size());
    m_trackedWindows.append(window);
    endInsertRows();

    connect(window, &AbstractWindow::pidChanged, this, [this, window]() {
        auto pos = m_trackedWindows.indexOf(window);
        auto modelIndex = index(pos);
        Q_EMIT dataChanged(modelIndex, modelIndex, {TaskManager::PidRole});
    });
    connect(window, &AbstractWindow::identityChanged, this, [this, window]() {
        auto pos = m_trackedWindows.indexOf(window);
        auto modelIndex = index(pos);
        Q_EMIT dataChanged(modelIndex, modelIndex, {TaskManager::IdentityRole});
    });
    connect(window, &AbstractWindow::iconChanged, this, [this, window]() {
        auto pos = m_trackedWindows.indexOf(window);
        auto modelIndex = index(pos);
        Q_EMIT dataChanged(modelIndex, modelIndex, {TaskManager::WinIconRole});
    });
    connect(window, &AbstractWindow::titleChanged, this, [this, window]() {
        auto pos = m_trackedWindows.indexOf(window);
        auto modelIndex = index(pos);
        Q_EMIT dataChanged(modelIndex, modelIndex, {TaskManager::WinTitleRole});
    });

    connect(window, &AbstractWindow::isActiveChanged, this, [this, window]() {
        auto pos = m_trackedWindows.indexOf(window);
        auto modelIndex = index(pos);
        Q_EMIT dataChanged(modelIndex, modelIndex, {TaskManager::ActiveRole});
    });
    connect(window, &AbstractWindow::shouldSkipChanged, this, [this, window]() {
        auto pos = m_trackedWindows.indexOf(window);
        auto modelIndex = index(pos);
        Q_EMIT dataChanged(modelIndex, modelIndex, {TaskManager::ShouldSkipRole});
    });
}

void AbstractWindowMonitor::destroyWindow(AbstractWindow * window)
{
    auto pos = m_trackedWindows.indexOf(window);
    if (pos == -1)
        return;

    beginRemoveRows(QModelIndex(), pos, pos);
    m_trackedWindows.removeAt(pos);
    endRemoveRows();
}


}
