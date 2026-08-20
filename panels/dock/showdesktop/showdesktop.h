// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "applet.h"
#include "dsglobal.h"
#include "treelandwindowmanager.h"

#include <DConfig>

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
    
    bool visible() const;
    void setVisible(bool visible);

Q_SIGNALS:
    void visibleChanged();

private slots:
    void onEnableShowDesktopChanged();

private:
    TreelandWindowManager *m_windowManager;
    Dtk::Core::DConfig *m_dockConfig;
    bool m_visible;
};

}
