// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "trashmonitor.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(trashMonitorLog, "org.deepin.dde.shell.dde-apps.trash")

namespace apps
{
TrashMonitor::TrashMonitor(QObject *parent)
    : QObject(parent)
    , m_trash(g_file_new_for_uri("trash:///"))
{
    updateState();

    GError *error = nullptr;
    m_monitor = g_file_monitor_file(m_trash, G_FILE_MONITOR_NONE, nullptr, &error);
    if (!m_monitor) {
        qCWarning(trashMonitorLog) << "Failed to monitor trash:"
                                   << (error ? error->message : "unknown error");
        g_clear_error(&error);
        return;
    }

    g_signal_connect(m_monitor, "changed", G_CALLBACK(onTrashChanged), this);
}

TrashMonitor::~TrashMonitor()
{
    if (m_monitor)
        g_object_unref(m_monitor);
    if (m_trash)
        g_object_unref(m_trash);
}

bool TrashMonitor::isEmpty() const
{
    return m_empty;
}

void TrashMonitor::updateState()
{
    GError *error = nullptr;
    GFileInfo *info = g_file_query_info(m_trash,
                                        G_FILE_ATTRIBUTE_TRASH_ITEM_COUNT,
                                        G_FILE_QUERY_INFO_NONE,
                                        nullptr,
                                        &error);
    if (!info) {
        qCWarning(trashMonitorLog) << "Failed to query trash item count:"
                                   << (error ? error->message : "unknown error");
        g_clear_error(&error);
        return;
    }

    const bool empty = g_file_info_get_attribute_uint32(info, G_FILE_ATTRIBUTE_TRASH_ITEM_COUNT) == 0;
    g_object_unref(info);

    if (m_empty == empty)
        return;

    m_empty = empty;
    Q_EMIT emptyChanged(m_empty);
}

void TrashMonitor::onTrashChanged(GFileMonitor *monitor,
                                  GFile *file,
                                  GFile *otherFile,
                                  GFileMonitorEvent eventType,
                                  gpointer userData)
{
    Q_UNUSED(monitor)
    Q_UNUSED(file)
    Q_UNUSED(otherFile)

    switch (eventType) {
    case G_FILE_MONITOR_EVENT_ATTRIBUTE_CHANGED:
    case G_FILE_MONITOR_EVENT_CHANGED:
    case G_FILE_MONITOR_EVENT_CREATED:
    case G_FILE_MONITOR_EVENT_DELETED:
        static_cast<TrashMonitor *>(userData)->updateState();
        break;
    default:
        break;
    }
}
}
