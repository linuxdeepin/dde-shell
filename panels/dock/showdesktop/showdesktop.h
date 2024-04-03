// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "applet.h"
#include "dsglobal.h"

DS_BEGIN_NAMESPACE
namespace dock {

class ShowDesktop : public DApplet
{
    Q_OBJECT
public:
    explicit ShowDesktop(QObject *parent = nullptr);
    virtual bool init() override;
    virtual bool load() override;

    Q_INVOKABLE void toggleShowDesktop();
    Q_INVOKABLE bool checkNeedShowDesktop();
};

}
DS_END_NAMESPACE
