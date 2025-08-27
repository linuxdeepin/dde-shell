// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "hoverpreviewproxymodel.h"
#include "taskmanager.h"
#include "taskmanagersettings.h"

#include <QDebug>

namespace dock
{

HoverPreviewProxyModel::HoverPreviewProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
    , m_settings(TaskManagerSettings::instance())
{
    // 设置动态排序，确保模型变化时自动重新过滤
    setDynamicSortFilter(true);
}

void HoverPreviewProxyModel::setFilterModelIndex(const QModelIndex &index)
{
    if (m_filterIndex == index)
        return;

    m_filterIndex = index;

    // 触发过滤器重新计算
    invalidateFilter();
}

void HoverPreviewProxyModel::clearFilter()
{
    setFilterModelIndex(QModelIndex());
}

bool HoverPreviewProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    Q_UNUSED(sourceParent)

    // 如果没有设置过滤条件，则不显示任何行
    if (!m_filterIndex.isValid())
        return false;

    if (!sourceModel())
        return false;

    QModelIndex sourceIndex = sourceModel()->index(sourceRow, 0);
    if (!sourceIndex.isValid())
        return false;

    // 确保有实际的窗口ID
    QVariant winIdData = sourceIndex.data(TaskManager::WinIdRole);
    uint32_t winId = winIdData.toUInt();
    if (winId == 0)
        return false;

    // 根据 noTaskGrouping 配置采用不同的过滤策略
    bool isWindowSplit = (m_settings && m_settings->isWindowSplit());

    if (isWindowSplit) {
        // noTaskGrouping = true: 精确匹配该模型索引对应的窗口
        // 比较窗口ID是否匹配
        QVariant filterWinId = m_filterIndex.data(TaskManager::WinIdRole);
        uint32_t targetWinId = filterWinId.toUInt();

        bool result = (winId == targetWinId && targetWinId != 0);
        // 在 WindowSplit 模式下，精确匹配单个窗口
        return result;
    } else {
        // noTaskGrouping = false: 基于 DesktopIdRole 匹配应用的所有窗口
        QVariant currentDesktopId = sourceIndex.data(TaskManager::DesktopIdRole);
        QVariant filterDesktopId = m_filterIndex.data(TaskManager::DesktopIdRole);

        bool result = (currentDesktopId.toString() == filterDesktopId.toString());
        // 在 Grouped 模式下，匹配同一应用的所有窗口
        return result;
    }
}

}
