// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "applet.h"
#include "dsglobal.h"

DS_BEGIN_NAMESPACE
namespace dock {

class ShowDesktop : public DApplet
{
    Q_OBJECT
    Q_PROPERTY(QString iconName READ iconName WRITE setIconName NOTIFY iconNameChanged FINAL)
public:
    explicit ShowDesktop(QObject *parent = nullptr);
    virtual bool init() override;

    QString iconName() const;
    void setIconName(const QString& iconName);

    Q_INVOKABLE void toggleShowDesktop();

Q_SIGNALS:
    void iconNameChanged();

private:
    QString m_iconName;
};

}
DS_END_NAMESPACE
