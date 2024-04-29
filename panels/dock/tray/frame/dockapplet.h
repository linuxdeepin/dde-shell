// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "applet.h"
#include "dsglobal.h"
#include "dbusdockadaptors.h"

#include <QSize>

class DockTrayWindow;
class QuickProxyWidget;
namespace dock {

class DockApplet : public DS_NAMESPACE::DApplet
{
    Q_OBJECT
    Q_PROPERTY(int dockWidth READ dockWidth WRITE setDockWidth NOTIFY dockWidthChanged)
    Q_PROPERTY(int dockHeight READ dockHeight WRITE setDockHeight NOTIFY dockHeightChanged)

public:
    explicit DockApplet(QObject *parent = nullptr);
    ~ DockApplet();

    void setDockWidth(int width);
    int dockWidth() const;

    void setDockHeight(int height);
    int dockHeight() const;

    Q_INVOKABLE void setPanelPosition(int x, int y) const;
    Q_INVOKABLE void setDockPosition(int pos) const;
    Q_INVOKABLE void setPanelSize(int size) const;
    Q_INVOKABLE void setDisplayMode(int displayMode) const;

    Q_INVOKABLE void initDock();
    Q_INVOKABLE void collapseExpandedPanel();

    virtual bool init() override;
    virtual bool load() override;


    // ------------ old dbus data for other module ----------------//
    Q_INVOKABLE DockItemInfos plugins();
    Q_INVOKABLE void setItemOnDock(const QString settingKey, const QString &itemKey, bool visible);

Q_SIGNALS:
    void dockWidthChanged(int);
    void dockHeightChanged(int);

private:
    QPointer<DockTrayWindow> m_window;
    OldDBusDock *m_dockAdapter;
    QuickProxyWidget *m_widgetProxy;
    int m_dockWidth;
    int m_dockHeight;
};

}
