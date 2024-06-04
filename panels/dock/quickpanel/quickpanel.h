// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "applet.h"
#include "dsglobal.h"

namespace dock {

class QuickPanelApplet : public DS_NAMESPACE::DApplet
{
    Q_OBJECT
public:
    explicit QuickPanelApplet(QObject *parent = nullptr);
};

}
