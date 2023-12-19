// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "applet.h"
#include "dsglobal.h"

DS_BEGIN_NAMESPACE
namespace dock {
const QStringList pluginDirs = {
    "/usr/lib/dde-dock/plugins/",
    "/usr/lib/dde-dock/plugins/quick-trays/",
    "/usr/lib/dde-dock/plugins/system-trays/"
};

class DockTray : public DApplet
{
    Q_OBJECT

public:
    explicit DockTray(QObject *parent = nullptr);
    virtual bool init() override;

};
}
DS_END_NAMESPACE
