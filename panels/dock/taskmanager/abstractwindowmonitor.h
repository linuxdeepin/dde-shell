// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dsglobal.h"
#include "abstractwindow.h"

#include <cstdint>

#include <QObject>

DS_BEGIN_NAMESPACE

namespace dock {
class AbstractWindowMonitor : public QObject
{
    Q_OBJECT

public:
    AbstractWindowMonitor(QObject* parent = nullptr): QObject(parent){};
    virtual void start() = 0;
    virtual void stop() = 0;

    virtual QPointer<AbstractWindow> getWindowByWindowId(ulong windowId) = 0;

    virtual void presentWindows(QStringList windows) = 0;

Q_SIGNALS:
    void windowAdded(QPointer<AbstractWindow> window);
    void WindowMonitorShutdown();
};
}
DS_END_NAMESPACE
