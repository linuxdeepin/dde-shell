// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QSortFilterProxyModel>

namespace dock
{

class TaskManagerSettings;

/**
 * @brief HoverPreviewProxyModel - 专用于任务栏 hover 预览的代理模型
 *
 * 该模型用于过滤出特定应用的所有窗口，确保预览窗口与实际窗口状态保持同步
 * 支持 noTaskGrouping 模式，在该模式下只显示特定窗口而非整个应用的窗口
 */
class HoverPreviewProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit HoverPreviewProxyModel(QObject *parent = nullptr);

    enum FilterMode {
        FilterByAppId,
        FilterByWinId,
    };

    /**
     * @brief 设置要预览的模型索引，在 noTaskGrouping 模式下精确匹配该窗口
     * @param index 要预览的模型索引，无效索引则清除过滤条件
     */
    void setFilter(QString filter, enum FilterMode mode);

    /**
     * @brief 清除过滤条件，重置模型状态
     */
    void clearFilter();

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    FilterMode m_filterMode;
    QString m_filter;
};

}
