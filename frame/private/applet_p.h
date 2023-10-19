// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "applet.h"

#include <dobject_p.h>
#include <QVariant>

DS_BEGIN_NAMESPACE
/**
 * @brief 插件项，单个插件实例
 */
class Q_DECL_EXPORT DAppletPrivate : public DTK_CORE_NAMESPACE::DObjectPrivate
{
public:
    DAppletPrivate(DApplet *qq )
        : DTK_CORE_NAMESPACE::DObjectPrivate(qq)
    {
    }

    DPluginMetaData m_metaData;

    D_DECLARE_PUBLIC(DApplet);
};

DS_END_NAMESPACE
