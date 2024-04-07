// Copyright (C) 2019 ~ 2019 Deepin Technology Co., Ltd.
// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DOCKITEMMANAGER_H
#define DOCKITEMMANAGER_H

#include "dockitem.h"
#include "pluginsiteminterface.h"

#include <QObject>
#include <QGSettings>

class PluginsItem;

/**
 * @brief The DockItemManager class
 * 管理类，管理所有的应用数据，插件数据
 */
class DockItemManager : public QObject
{
    Q_OBJECT

public:
    static DockItemManager *instance(QObject *parent = nullptr);

    const QList<QPointer<DockItem> > itemList() const;

signals:
    void itemInserted(const int index, DockItem *item) const;
    void itemRemoved(DockItem *item) const;
    void itemUpdated(DockItem *item) const;
    void trayVisableCountChanged(const int &count) const;
    void requestWindowAutoHide(const bool autoHide) const;
    void requestRefershWindowVisible() const;

    void requestUpdateDockItem() const;

public slots:
    void refreshItemsIcon();
    void itemMoved(DockItem *const sourceItem, DockItem *const targetItem);

private Q_SLOTS:
    void onPluginLoadFinished();
    void onPluginItemRemoved(PluginsItemInterface *itemInter);
    void onPluginUpdate(PluginsItemInterface *itemInter);

private:
    explicit DockItemManager(QObject *parent = nullptr);
    void updatePluginsItemOrderKey();
    void manageItem(DockItem *item);
    void pluginItemInserted(PluginsItem *item);

private:
    static DockItemManager *INSTANCE;

    QList<QPointer<DockItem>> m_itemList;
    QList<QString> m_appIDist;
    QList<PluginsItemInterface *> m_pluginItems;

    bool m_loadFinished; // 记录所有插件是否加载完成

    static const QGSettings *m_appSettings;
    static const QGSettings *m_activeSettings;
    static const QGSettings *m_dockedSettings;
};

#endif // DOCKITEMMANAGER_H
