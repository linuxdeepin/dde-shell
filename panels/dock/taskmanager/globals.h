// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QRegularExpression>

namespace dock {

// actions
static inline const QString DOCK_ACTION_ALLWINDOW = "dock-action-allWindow";
static inline const QString DOCK_ACTION_FORCEQUIT = "dock-action-forceQuit";
static inline const QString DOCK_ACTION_CLOSEALL = "dock-action-closeAll";
static inline const QString DOCK_ACTION_CLOSEWINDOW = "dock-action-closeWindow";
static inline const QString DOCK_ACTIN_LAUNCH = "dock-action-launch";
static inline const QString DOCK_ACTION_DOCK = "dock-action-dock";

// setting keys
static inline const QString TASKMANAGER_ALLOWFOCEQUIT_KEY = "Allow_Force_Quit";
static inline const QString TASKMANAGER_WINDOWSPLIT_KEY = "noTaskGrouping";
static inline const QString TASKMANAGER_CGROUPS_BASED_GROUPING_KEY = "cgroupsBasedGrouping";
static inline const QString TASKMANAGER_CGROUPS_BASED_GROUPING_SKIP_APPIDS = "cgroupsBasedGroupingSkipAppIds";
static inline const QString TASKMANAGER_CGROUPS_BASED_GROUPING_SKIP_CATEGORIES = "cgroupsBasedGroupingSkipCategories";
static inline const QString TASKMANAGER_DOCKEDITEMS_KEY = "Docked_Items";
constexpr auto TASKMANAGER_DOCKEDELEMENTS_KEY = "dockedElements";

// model roleNames
constexpr auto MODEL_WINID = "winId";
constexpr auto MODEL_PID = "pid";
constexpr auto MODEL_IDENTIFY = "identity";
constexpr auto MODEL_WINICON = "icon";
constexpr auto MODEL_TITLE = "title";
constexpr auto MODEL_ACTIVE = "active";
constexpr auto MODEL_ATTENTION = "attention";
constexpr auto MODEL_SHOULDSKIP = "shouldSkip";

constexpr auto MODEL_DESKTOPID = "desktopId";
constexpr auto MODEL_ICONNAME = "iconName";
constexpr auto MODEL_ITEMID = "itemId";
constexpr auto MODEL_NAME = "name";
constexpr auto MODEL_STARTUPWMCLASS = "startupWMClass";
constexpr auto MODEL_ACTIONS = "actions";
constexpr auto MODEL_MENUS = "menus";
constexpr auto MODEL_DOCKED = "docked";
constexpr auto MODEL_WINDOWS = "windows";

// default item icon
constexpr auto DEFAULT_APP_ICONNAME = "application-default-icon";

// copy from application-manager

inline static QString escapeToObjectPath(const QString &str)
{
    if (str.isEmpty()) {
        return "_";
    }

    auto ret = str;
    QRegularExpression re{R"([^a-zA-Z0-9])"};
    auto matcher = re.globalMatch(ret);
    while (matcher.hasNext()) {
        auto replaceList = matcher.next().capturedTexts();
        replaceList.removeDuplicates();
        for (const auto &c : replaceList) {
            auto hexStr = QString::number(static_cast<uint>(c.front().toLatin1()), 16);
            ret.replace(c, QString{R"(_%1)"}.arg(hexStr));
        }
    }
    return ret;
}

inline static QString unescapeFromObjectPath(const QString &str)
{
    auto ret = str;
    for (qsizetype i = 0; i < str.size(); ++i) {
        if (str[i] == '_' and i + 2 < str.size()) {
            auto hexStr = str.sliced(i + 1, 2);
            ret.replace(QString{"_%1"}.arg(hexStr), QChar::fromLatin1(hexStr.toUInt(nullptr, 16)));
            i += 2;
        }
    }
    return ret;
}
}
