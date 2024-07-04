// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "private/dsqmlglobal_p.h"

#include "applet.h"
#include "containment.h"
#include "pluginloader.h"

#include <QLoggingCategory>
#include <QCoreApplication>
#include <QQueue>
#include <QWindow>
#include <QGuiApplication>

#include <dobject_p.h>

DS_BEGIN_NAMESPACE
DCORE_USE_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(dsLog)

class DQmlGlobalPrivate : public DObjectPrivate
{
public:
    explicit DQmlGlobalPrivate(DQmlGlobal *qq)
        : DObjectPrivate(qq)
    {
    }

    D_DECLARE_PUBLIC(DQmlGlobal)
};

DQmlGlobal::DQmlGlobal(QObject *parent)
    : QObject(parent)
    , DObject(*new DQmlGlobalPrivate(this))
{
}

DQmlGlobal::~DQmlGlobal()
{
}

DApplet *DQmlGlobal::applet(const QString &pluginId) const
{
    D_DC(DQmlGlobal);
    const auto list = appletList(pluginId);
    if (!list.isEmpty())
        return list.first();
    return nullptr;
}

QList<DApplet *> DQmlGlobal::appletList(const QString &pluginId) const
{
    D_DC(DQmlGlobal);
    QList<DApplet *> ret;
    auto root = qobject_cast<DContainment *>(rootApplet());

    QQueue<DContainment *> containments;
    containments.enqueue(root);
    while (!containments.isEmpty()) {
        DContainment *containment = containments.dequeue();
        for (const auto applet : containment->applets()) {
            if (auto item = qobject_cast<DContainment *>(applet)) {
                containments.enqueue(item);
            }
            if (applet->pluginId() == pluginId)
                ret << applet;
        }
    }
    return ret;
}

QList<QWindow *> DQmlGlobal::allChildrenWindows(QWindow *target)
{
    QList<QWindow *> ret;
    auto allWindows = qGuiApp->allWindows();
    while (!allWindows.isEmpty()) {
        auto window = allWindows.takeFirst();
        while (window) {
            if (window->transientParent() == target) {
                ret << window;
                break;
            }
            window = window->transientParent();
        }
    }
    return ret;
}

void DQmlGlobal::closeChildrenWindows(QWindow *target)
{
    for (const auto item : allChildrenWindows(target))
        if (item && item->isVisible())
            item->close();
}

DApplet *DQmlGlobal::rootApplet() const
{
    return DPluginLoader::instance()->rootApplet();
}

DQmlGlobal *DQmlGlobal::instance()
{
    static DQmlGlobal *gInstance = nullptr;
    if (!gInstance) {
        gInstance = new DQmlGlobal();
        // ensure DQmlGlobal is live in main thread.
        gInstance->moveToThread(qApp->thread());
    }
    return gInstance;
}

DS_END_NAMESPACE
