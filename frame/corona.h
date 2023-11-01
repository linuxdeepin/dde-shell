// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dsglobal.h"
#include "containment.h"

#include <QQuickWindow>

DS_BEGIN_NAMESPACE

/**
 * @brief 插件集
 */
class DCoronaPrivate;
class DS_SHARE DCorona : public DContainment
{
    Q_OBJECT
    D_DECLARE_PRIVATE(DCorona)
public:
    explicit DCorona(QObject *parent = nullptr);
    virtual ~DCorona() override;

    QQuickWindow *window() const;

    // 加载插件
    virtual void load() override;
    // 初始化
    virtual void init() override;
};

DS_END_NAMESPACE
