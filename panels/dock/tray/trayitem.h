// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "applet.h"
#include "dsglobal.h"
#include "dockiteminfo.h"

#include <QAbstractItemModel>

namespace dock {

class TrayItem : public DS_NAMESPACE::DApplet
{
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel* trayPluginModel READ trayPluginModel WRITE setTrayPluginModel NOTIFY trayPluginModelChanged FINAL)
public:
    explicit TrayItem(QObject *parent = nullptr);

    QAbstractItemModel *trayPluginModel() const;
    void setTrayPluginModel(QAbstractItemModel *newTrayPluginModel);

    Q_INVOKABLE DockItemInfos dockItemInfos();
    Q_INVOKABLE void setItemOnDock(const QString &settingKey, const QString &surfaceId, bool visible);

Q_SIGNALS:
    void trayPluginModelChanged();

private:
    QAbstractItemModel *m_trayPluginModel = nullptr;
    DockItemInfos m_itemInfos;
};

}
