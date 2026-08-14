// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

#undef signals
#include <gio/gio.h>
#define signals Q_SIGNALS

namespace apps
{
class TrashMonitor : public QObject
{
    Q_OBJECT

public:
    explicit TrashMonitor(QObject *parent = nullptr);
    ~TrashMonitor() override;

    bool isEmpty() const;

signals:
    void emptyChanged(bool empty);

private:
    void updateState();
    static void onTrashChanged(GFileMonitor *monitor,
                               GFile *file,
                               GFile *otherFile,
                               GFileMonitorEvent eventType,
                               gpointer userData);

    GFile *m_trash = nullptr;
    GFileMonitor *m_monitor = nullptr;
    bool m_empty = true;
};
}
