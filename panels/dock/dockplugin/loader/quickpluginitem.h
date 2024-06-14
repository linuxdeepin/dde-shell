// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef QUICK_PLUGIN_ITEM_H
#define QUICK_PLUGIN_ITEM_H

#include "pluginitem.h"

class QuickPluginItem : public PluginItem
{
    Q_OBJECT
public:
    explicit QuickPluginItem(PluginsItemInterface *pluginInterface, const QString &itemKey, QWidget *parent = nullptr);
    ~QuickPluginItem() override = default;

protected:
    virtual QWidget *centralWidget() override;

private:
};

#endif
