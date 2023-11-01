// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "corona.h"

DS_USE_NAMESPACE

class ExampleCorona : public DCorona
{
    Q_OBJECT
public:
    explicit ExampleCorona(QObject *parent = nullptr);

    void load() override;

    virtual void init() override;
};
