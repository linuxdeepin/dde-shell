// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "xworkspaceworker.h"
#include "workspacemodel.h"

#include <QDBusReply>
#include <QGuiApplication>
#include <QScreen>
#include <QMetaMethod>
#include <QDBusConnectionInterface>
#include <QTimer>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(workspaceItem, "dde.shell.dock.workspaceItem")

DS_BEGIN_NAMESPACE
namespace dock {

XWorkspaceWorker::XWorkspaceWorker(WorkspaceModel *model)
    : QObject(model)
    , m_inter(new QDBusInterface("com.deepin.wm", "/com/deepin/wm", "com.deepin.wm"))
    , m_model(model)
{
    updateData();

    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        qDebug() << "DBus session bus connection failed.";
        return;
    }

    bus.connect("com.deepin.wm", "/com/deepin/wm", "com.deepin.wm", "workspaceCountChanged", this, SLOT(updateData()));
    bus.connect("com.deepin.wm", "/com/deepin/wm", "com.deepin.wm", "WorkspaceSwitched", this, SLOT(updateData()));
    bus.connect("com.deepin.wm", "/com/deepin/wm", "com.deepin.wm", "WorkspaceBackgroundChangedForMonitor", this, SLOT(updateData()));
    bus.connect("com.deepin.wm", "/com/deepin/wm", "com.deepin.wm", "WorkspaceBackgroundChanged", this, SLOT(updateData()));
    connect(m_model, &WorkspaceModel::currentIndexChanged, this, &XWorkspaceWorker::setIndex);
}

void XWorkspaceWorker::setIndex(int index)
{
    QDBusReply<int> resIndex = m_inter->call("GetCurrentWorkspace");
    if (resIndex.error().message().isEmpty() && m_model->currentIndex() + 1 != resIndex.value()) {
        m_inter->call("SetCurrentWorkspace", index + 1);
    } else {
        qCWarning(workspaceItem) << "GetCurrentWorkspace failed:" << resIndex.error().message();
        return;
    }
}

void XWorkspaceWorker::updateData()
{
    int count = 0;
    QDBusReply<int> resCount = m_inter->call("WorkspaceCount");
    if (resCount.error().message().isEmpty()) {
        count = resCount.value();
    } else {
        qCWarning(workspaceItem) << "WorkspaceCount failed:" << resCount.error().message();
        return;
    }

    QList<WorkSpaceData*> items;
    for (int i = 1; i <= count; ++i) {
        QString image;
        QDBusReply<QString> res = m_inter->call("GetWorkspaceBackgroundForMonitor", i, QGuiApplication::primaryScreen()->name());
        if (res.error().message().isEmpty()) {
            image = res.value();
        } else {
            qCWarning(workspaceItem) << "GetWorkspaceBackgroundForMonitor failed:" << res.error().message();
            return;
        }
        auto item = new WorkSpaceData(image);
        items.append(item);
    }
    m_model->setItems(items);
    QDBusReply<int> index = m_inter->call("GetCurrentWorkspace");
    if (index.error().message().isEmpty()) {
        qWarning() << "m_model->setCurrentIndex(index.value() - 1) " <<  index.value();
        m_model->setCurrentIndex(index.value() - 1);
    } else {
        qCWarning(workspaceItem) << "GetCurrentWorkspace failed:" << index.error().message();
        return;
    }
}
}
DS_END_NAMESPACE
