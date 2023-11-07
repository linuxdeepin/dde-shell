// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dsglobal.h"

#include <QObject>
#include <QVariant>

DS_BEGIN_NAMESPACE
namespace dock {
class DockAbstractConfig : public QObject
{
    Q_OBJECT
public:
    DockAbstractConfig(QObject* parent = nullptr) : QObject(parent) {};

    virtual QVariant value(const QString& key) = 0;
    virtual void setValue(const QString& key, const QVariant& value) =0;

    virtual bool isValid() {return false;};

Q_SIGNALS:
    void valueChanged(const QString& key);
};
}

DS_END_NAMESPACE
