// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// from am global.h
#include <QMap>
#include <QString>
#include <QStringList>
#include <QDBusObjectPath>

using ObjectInterfaceMap = QMap<QString, QVariantMap>;
using ObjectMap = QMap<QDBusObjectPath, ObjectInterfaceMap>;
using QStringMap = QMap<QString, QString>;
// Match AM's locale -> keyword list D-Bus type without flattening locale information.
using QStringListMap = QMap<QString, QStringList>;
using PropMap = QMap<QString, QStringMap>;

Q_DECLARE_METATYPE(ObjectInterfaceMap)
Q_DECLARE_METATYPE(ObjectMap)
Q_DECLARE_METATYPE(QStringMap)
Q_DECLARE_METATYPE(QStringListMap)
Q_DECLARE_METATYPE(PropMap)
