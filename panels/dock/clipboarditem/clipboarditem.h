// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "applet.h"
#include "dsglobal.h"

DS_BEGIN_NAMESPACE
namespace dock {

class ClipboardItem : public DApplet
{
    Q_OBJECT
public:
    explicit ClipboardItem(QObject *parent = nullptr);

    Q_INVOKABLE void toggleClipboard();
};

}
DS_END_NAMESPACE
