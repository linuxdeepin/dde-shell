// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dde-control-center/dccfactory.h"
#include "operation/dockpluginmodel.h"

#include "dccdockexport.h"
#include <QFile>
#include <QIcon>

static const QMap<QString, QString> pluginIconMap = {
    {"AiAssistant",    "dcc_dock_assistant"}
    , {"show-desktop",   "dcc_dock_desktop"}
    , {"onboard",        "dcc_dock_keyboard"}
    , {"notifications",  "dcc_dock_notify"}
    , {"shutdown",       "dcc_dock_power"}
    , {"multitasking",   "dcc_dock_task"}
    , {"datetime",       "dcc_dock_time"}
    , {"system-monitor", "dcc_dock_systemmonitor"}
    , {"grand-search",   "dcc_dock_grandsearch"}
    , {"trash",          "dcc_dock_trash"}
    , {"shot-start-plugin",  "dcc_dock_shot_start_plugin"}
};

DccDockExport::DccDockExport(QObject *parent) 
: QObject(parent)
, m_pluginModel(new DockPluginModel(this))
, m_dockDbusProxy(new DockDBusProxy(this))
{
    initData();
}

void DccDockExport::initData()
{
    QDBusPendingReply<DockItemInfos> pluginInfos = m_dockDbusProxy->plugins();
    auto infos = pluginInfos.value();
    for (auto &info : infos) {
        QString pluginIconStr;
        if (QFile::exists(info.dcc_icon)) {
            pluginIconStr = info.dcc_icon;
        } else if (pluginIconMap.contains(info.itemKey)) {
            pluginIconStr = pluginIconMap.value(info.itemKey);
        } else {
            pluginIconStr = info.itemKey;
        }
        QIcon tmpIcon = QIcon::fromTheme(pluginIconStr);

        if (tmpIcon.isNull()) {
            pluginIconStr = "dcc_dock_plug_in";
        }
        info.dcc_icon = pluginIconStr;
    }
    m_pluginModel->resetData(infos);

    connect(m_dockDbusProxy, &DockDBusProxy::pluginVisibleChanged, m_pluginModel, &DockPluginModel::setPluginVisible);
}

DCC_FACTORY_CLASS(DccDockExport)

#include "dccdockexport.moc"
