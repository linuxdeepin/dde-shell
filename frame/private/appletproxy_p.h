// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "appletproxy.h"

#include <dobject_p.h>
#include <QVariant>
#include <QPointer>

DS_BEGIN_NAMESPACE
/**
 * @brief
 */
class DAppletProxyPrivate : public DTK_CORE_NAMESPACE::DObjectPrivate
{
public:
    DAppletProxyPrivate(DAppletProxy *qq);
    ~DAppletProxyPrivate() override;

    D_DECLARE_PUBLIC(DAppletProxy);
};

DS_END_NAMESPACE
