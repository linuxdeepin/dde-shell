// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dsglobal.h"

#include <QObject>
#include <DConfig>
#include <QScopedPointer>

DCORE_USE_NAMESPACE

DS_BEGIN_NAMESPACE

namespace dock {

class TaskManagerSettings : public QObject
{
    Q_OBJECT

public:
    static TaskManagerSettings* instance();

    bool isAllowedForceQuit();
    void setAllowedForceQuit(bool allowed);

    bool isWindowSplit();
    void setWindowSplit(bool split);

private:
    explicit TaskManagerSettings(QObject *parent = nullptr);

Q_SIGNALS:
    void allowedForceQuitChanged();
    void windowSplitChanged();

private:
    DConfig* m_taskManagerDconfig;

    bool m_allowForceQuit;
    bool m_windowSplit;
};
}
DS_END_NAMESPACE
