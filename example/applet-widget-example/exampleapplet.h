// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "applet.h"

DS_USE_NAMESPACE

class WidgetExampleApplet : public DApplet
{
    Q_OBJECT
public:
    explicit WidgetExampleApplet(QObject *parent = nullptr);

    virtual bool init() override;
};
