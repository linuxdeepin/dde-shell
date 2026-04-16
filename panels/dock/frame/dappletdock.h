// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "applet.h"
#include "dockiteminfo.h"
#include "dsglobal.h"

DS_BEGIN_NAMESPACE

class DAppletDockPrivate;

class DS_SHARE DAppletDock : public DApplet
{
    Q_OBJECT
    Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY visibleChanged)
    D_DECLARE_PRIVATE(DAppletDock)

public:
    explicit DAppletDock(QObject *parent = nullptr);
    virtual ~DAppletDock() = default;

    virtual DockItemInfo dockItemInfo();

    bool visible() const;
    void setVisible(bool visible);

Q_SIGNALS:
    void visibleChanged();

protected:
    explicit DAppletDock(DAppletDockPrivate &dd, QObject *parent = nullptr);
};

DS_END_NAMESPACE
