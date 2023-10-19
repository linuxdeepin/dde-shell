// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "qmlplugin.h"
#include "applet.h"
#include "appletitem.h"
#include "containmentitem.h"
#include "dstypes.h"

DS_BEGIN_NAMESPACE

void QmlpluginPlugin::registerTypes(const char *uri)
{
    // @uri org.deepin.ds
    qmlRegisterModule(uri, 1, 0);

    qmlRegisterUncreatableType<Types>(uri, 1, 0, "Types", "Types");

    qmlRegisterAnonymousType<DApplet>(uri, 1);
    qmlRegisterType<DAppletItem>(uri, 1, 0, "AppletItem");
    qmlRegisterUncreatableType<DAppletItem>(uri, 1, 0, "Applet", "Applet Attached");
    qmlRegisterType<DContainmentItem>(uri, 1, 0, "ContainmentItem");
    qmlRegisterUncreatableType<DContainmentItem>(uri, 1, 0, "Containment", "Containment Attached");
}

void QmlpluginPlugin::initializeEngine(QQmlEngine *engine, const char *uri)
{
    QQmlExtensionPlugin::initializeEngine(engine, uri);
}

DS_END_NAMESPACE
