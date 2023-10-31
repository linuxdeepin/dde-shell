// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "containment_p.h"
#include "corona.h"

#include <dobject_p.h>
#include <QVariant>

DS_BEGIN_NAMESPACE
/**
 * @brief 插件项
 */
class DCoronaPrivate : public DContainmentPrivate
{
public:
    explicit DCoronaPrivate(DCorona *qq)
        : DContainmentPrivate(qq)
    {

    }

    void initDciSearchPaths();

    QQuickWindow *m_window = nullptr;

    D_DECLARE_PUBLIC(DCorona)
};

DS_END_NAMESPACE

