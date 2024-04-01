// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "applet.h"
#include "dsglobal.h"
#include "../dockiteminfo.h"

DS_BEGIN_NAMESPACE
namespace dock {

class SearchItem : public DApplet
{
    Q_OBJECT
    Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY visibleChanged)
public:
    explicit SearchItem(QObject *parent = nullptr);

    Q_INVOKABLE void toggleGrandSearch();
    Q_INVOKABLE void toggleGrandSearchConfig();

    Q_INVOKABLE DockItemInfo dockItemInfo();

    inline bool visible() const { return m_visible;}
    Q_INVOKABLE void setVisible(bool visible);

Q_SIGNALS:
    void visibleChanged(bool);

private:
    bool m_visible;
};

}
DS_END_NAMESPACE
