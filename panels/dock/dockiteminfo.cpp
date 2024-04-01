// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dockiteminfo.h"

#include <QDBusMetaType>

QDebug operator<<(QDebug argument, const DockItemInfo &info)
{
    argument << "name:" << info.name << ", displayName:" << info.displayName
             << "itemKey:" << info.itemKey << "SettingKey:" << info.settingKey
             << "icon_light:" << info.iconLight << "icon_dark:" << info.iconDark << "visible:" << info.visible;
    return argument;
}

QDBusArgument &operator<<(QDBusArgument &arg, const DockItemInfo &info)
{
    arg.beginStructure();
    arg << info.name << info.displayName << info.itemKey << info.settingKey << info.iconLight << info.iconDark << info.visible;
    arg.endStructure();
    return arg;
}

const QDBusArgument &operator>>(const QDBusArgument &arg, DockItemInfo &info)
{
    arg.beginStructure();
    arg >> info.name >> info.displayName >> info.itemKey >> info.settingKey >> info.iconLight >> info.iconDark >> info.visible;
    arg.endStructure();
    return arg;
}

void registerPluginInfoMetaType()
{
    qRegisterMetaType<DockItemInfo>("DockItemInfo");
    qDBusRegisterMetaType<DockItemInfo>();
    qRegisterMetaType<DockItemInfos>("DockItemInfos");
    qDBusRegisterMetaType<DockItemInfos>();
}
