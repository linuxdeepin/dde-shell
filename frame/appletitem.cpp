// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "appletitem.h"
#include "private/appletitem_p.h"
#include "private/applet_p.h"
#include "applet.h"
#include "qmlengine.h"

#include <dobject_p.h>
#include <QLoggingCategory>
#include <QQmlEngine>
#include <QQmlContext>
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

    std::unique_ptr<DQmlEngine> engine(new DQmlEngine(applet, applet));

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
    item->d_func()->m_engine = engine.release();
    g_appletItems[applet] = item;

    item->d_func()->m_engine->completeCreate();
    applet->d_func()->setRootObject(item);

    return item;
}

DApplet *DAppletItem::qmlAttachedProperties(QObject *object)
{
    if (auto context = qmlContext(object)) {
        return context->contextProperty("_ds_applet").value<DApplet *>();
    }
    return nullptr;
}

DS_END_NAMESPACE
