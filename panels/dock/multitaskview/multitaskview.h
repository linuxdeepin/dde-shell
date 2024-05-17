// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ddockapplet.h"
#include "dsglobal.h"

namespace dock {

class MultiTaskView : public DS_NAMESPACE::DDockApplet
{
    Q_OBJECT

public:
    explicit MultiTaskView(QObject *parent = nullptr);
    virtual bool init() override;

    bool hasComposite();

    QString displayName() const override;
    QString itemKey() const override;
    QString settingKey() const override;

    Q_INVOKABLE void openWorkspace();

public Q_SLOTS:
    void onHasCompositeChanged();
};

}
