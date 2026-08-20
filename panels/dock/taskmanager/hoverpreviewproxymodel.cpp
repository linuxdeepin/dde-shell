// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "hoverpreviewproxymodel.h"
#include "taskmanager.h"

#include <QDebug>

namespace dock
{

HoverPreviewProxyModel::HoverPreviewProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    // 设置动态排序，确保模型变化时自动重新过滤
    setDynamicSortFilter(true);
}

void HoverPreviewProxyModel::setFilter(QString filter, enum FilterMode mode)
{
    m_filter = filter;
    m_filterMode = mode;

    invalidateFilter();
}

void HoverPreviewProxyModel::clearFilter()
{
    setFilter(QString(), FilterByAppId);
}

bool HoverPreviewProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    Q_UNUSED(sourceParent)

    // 如果没有设置过滤条件，则不显示任何行
    if (m_filter.isEmpty())
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

    switch (m_filterMode) {
    case FilterByAppId: {
        QString currentDesktopId = sourceIndex.data(TaskManager::DesktopIdRole).toString();
        return currentDesktopId == m_filter;
    }
    case FilterByWinId: {
        uint32_t targetWinId = m_filter.toUInt();
        bool result = (winId == targetWinId && targetWinId != 0);
        return result;
    }
    }

    return false;
}

}
