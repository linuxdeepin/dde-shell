// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "panelview.h"
#include "examplepanel.h"
#include "pluginloader.h"

#include <QGuiApplication>
#include <QDebug>

DS_USE_NAMESPACE;

int main(int argc, char *argv[])
{
    QGuiApplication a(argc, argv);

    DPluginLoader::instance()->addPackageDir("/home/repo/dde-shell/example");

    ExamplePanel panel;
    panel.load();

    auto view = panel.view();
    view->resize(600, 400);
    view->show();

    return a.exec();
}
