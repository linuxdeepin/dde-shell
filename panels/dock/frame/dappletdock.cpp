// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dappletdock.h"
#include <private/applet_p.h>

DS_BEGIN_NAMESPACE

class DAppletDockPrivate : public DAppletPrivate
{
public:
    explicit DAppletDockPrivate(DAppletDock *qq)
        : DAppletPrivate(qq)
    {
    }
    ~DAppletDockPrivate() override = default;

    bool m_visible = true;

    D_DECLARE_PUBLIC(DAppletDock)
};

DAppletDock::DAppletDock(QObject *parent)
    : DApplet(*new DAppletDockPrivate(this), parent)
{
}

DAppletDock::DAppletDock(DAppletDockPrivate &dd, QObject *parent)
    : DApplet(dd, parent)
{
}

bool DAppletDock::visible() const
{
    D_DC(DAppletDock);
    return d->m_visible;
}

void DAppletDock::setVisible(bool visible)
{
    D_D(DAppletDock);
    if (d->m_visible == visible)
        return;

    d->m_visible = visible;
    Q_EMIT visibleChanged();
}

DockItemInfo DAppletDock::dockItemInfo()
{
    return {};
}

DS_END_NAMESPACE
