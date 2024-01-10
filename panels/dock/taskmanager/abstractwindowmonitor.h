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

    virtual void presentWindows(QList<uint32_t> windowsId) = 0;
    virtual void showWindowsPreview(QList<uint32_t> windowsId, QObject* relativePositionItem, int32_t previewXoffset, int32_t previewYoffset, uint32_t direction) = 0;
    virtual void hideWindowsPreview() = 0;

Q_SIGNALS:
    void windowAdded(QPointer<AbstractWindow> window);
    void WindowMonitorShutdown();
};
}
DS_END_NAMESPACE
