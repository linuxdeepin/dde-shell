// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "private/applet_p.h"
#include "ddockapplet.h"

#include <dobject_p.h>
#include <QVariant>

DS_BEGIN_NAMESPACE
class DDockAppletPrivate : public DAppletPrivate
{
public:
    explicit DDockAppletPrivate(DDockApplet *qq);
    ~DDockAppletPrivate() override;

    bool visible = false;
    QString icon;
    D_DECLARE_PUBLIC(DDockApplet);
};

DS_END_NAMESPACE
