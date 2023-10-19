// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "applet.h"

DS_USE_NAMESPACE

class Q_DECL_EXPORT ExampleApplet : public DApplet
{
    Q_OBJECT
    Q_PROPERTY(QString mainText READ mainText CONSTANT)
public:
    ExampleApplet(QObject *parent = nullptr);

    QString mainText() const;
private:
    QString m_mainText;
};
