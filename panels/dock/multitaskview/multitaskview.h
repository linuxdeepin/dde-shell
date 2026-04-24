// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../frame/dappletdock.h"
#include "../frame/dockiteminfo.h"
#include "applet.h"
#include "dsglobal.h"
#include "treelandmultitaskview.h"

#include <DConfig>

namespace dock {

class MultiTaskView : public DS_NAMESPACE::DAppletDock
{
    Q_OBJECT
    Q_PROPERTY(QString iconName READ iconName WRITE setIconName NOTIFY iconNameChanged FINAL)

public:
    explicit MultiTaskView(QObject *parent = nullptr);
    virtual bool init() override;

    QString iconName() const;
    void setIconName(const QString& iconName);
    bool hasComposite();

    Q_INVOKABLE void openWorkspace();
    DockItemInfo dockItemInfo() override;

Q_SIGNALS:
    void iconNameChanged();

private:
    void queryKwinVersion();
    bool m_kWinEffect = true;
    QString m_kwinVersion;
    QString m_iconName;
    QScopedPointer<TreeLandMultitaskview> m_multitaskview;
    Dtk::Core::DConfig *m_kWinCompositingConfig = nullptr;
};

}
