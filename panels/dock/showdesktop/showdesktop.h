// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "applet.h"
#include "dsglobal.h"
#include "../dockiteminfo.h"

namespace dock {

class ShowDesktop : public DS_NAMESPACE::DApplet
{
    Q_OBJECT
    Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY visibleChanged)

public:
    explicit ShowDesktop(QObject *parent = nullptr);
    virtual bool init() override;
    virtual bool load() override;

    Q_INVOKABLE void toggleShowDesktop();
    Q_INVOKABLE bool checkNeedShowDesktop();

    Q_INVOKABLE DockItemInfo dockItemInfo();
    Q_INVOKABLE bool visible() const;
    Q_INVOKABLE void setVisible(bool visible);

Q_SIGNALS:
    void visibleChanged();

private:
    bool m_visible;
};

}
