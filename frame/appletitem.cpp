// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "appletitem.h"
#include "private/appletitem_p.h"
#include "applet.h"
#include "qmlengine.h"

#include <dobject_p.h>
#include <QLoggingCategory>
#include <QQmlEngine>
#include <QQuickWindow>

DS_BEGIN_NAMESPACE
DCORE_USE_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(dsLog)

static QMap<DApplet *, DAppletItem *> g_appletItems;

DAppletItem::DAppletItem(QQuickItem *parent)
    : QQuickItem(parent)
    , DObject(*new DAppletItemPrivate(this))
{
}

DAppletItem::~DAppletItem()
{
}

DApplet *DAppletItem::applet() const
{
    D_DC(DAppletItem);
    return d->m_applet;
}

DAppletItem *DAppletItem::itemForApplet(DApplet *applet)
{
    auto it = g_appletItems.constFind(applet);
    if (it != g_appletItems.constEnd())
        return it.value();

    QScopedPointer<DQmlEngine> engine(new DQmlEngine(applet, applet));

    auto rootObject = engine->beginCreate();
    if (!rootObject) {
        return nullptr;
    }

    auto item = qobject_cast<DAppletItem *>(rootObject);
    if (!item) {
        rootObject->deleteLater();
        return nullptr;
    }

    item->d_func()->m_applet = applet;
    item->d_func()->m_engine = engine.take();
    g_appletItems[applet] = item;

    item->d_func()->m_engine->completeCreate();

    return item;
}

DApplet *DAppletItem::qmlAttachedProperties(QObject *object)
{
    QQuickItem *item = qobject_cast<QQuickItem *>(object);
    while (item) {
        if (auto appletItem = qobject_cast<DAppletItem *>(item)) {
            return appletItem->applet();
        }
        item = item->parentItem();
    }
    if (!item) {
        item = qobject_cast<QQuickItem *>(object);
        if(auto applet = item->window()->property("_ds_window_applet").value<DApplet *>()) {
            return applet;
        }
    }
    return nullptr;
}

DS_END_NAMESPACE
