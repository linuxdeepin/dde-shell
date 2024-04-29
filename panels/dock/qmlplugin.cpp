// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "qmlplugin.h"
#include "ddockapplet.h"
#include "ddockappletitem.h"
#include "layershell/dlayershellwindow.h"

#include <qqml.h>

QML_DECLARE_TYPEINFO(DS_NAMESPACE::DLayerShellWindow, QML_HAS_ATTACHED_PROPERTIES)

DS_BEGIN_NAMESPACE

void QmlpluginPlugin::registerTypes(const char *uri)
{
    // @uri org.deepin.ds.dockshell
    qmlRegisterModule(uri, 1, 0);

    qmlRegisterAnonymousType<DDockApplet>(uri, 1);
    qmlRegisterType<DDockAppletItem>(uri, 1, 0, "DDockAppletItem");
    qmlRegisterUncreatableType<DDockAppletItem>(uri, 1, 0, "DDockApplet", "DDockApplet Attached");
}

void QmlpluginPlugin::initializeEngine(QQmlEngine *engine, const char *uri)
{
    QQmlExtensionPlugin::initializeEngine(engine, uri);
}

DS_END_NAMESPACE
