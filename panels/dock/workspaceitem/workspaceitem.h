// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "applet.h"
#include "dsglobal.h"

DS_BEGIN_NAMESPACE
namespace dock {

class WorkspaceItem : public DApplet
{
    Q_OBJECT
public:
    explicit WorkspaceItem(QObject *parent = nullptr);

    Q_INVOKABLE void toggleWorkspace();
};

}
DS_END_NAMESPACE
