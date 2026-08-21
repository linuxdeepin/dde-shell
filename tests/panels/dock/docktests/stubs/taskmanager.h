// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Minimal stub of panels/dock/taskmanager/taskmanager.h for unit testing
// hoverpreviewproxymodel without pulling in the full DTK containment/applet
// dependency chain. Only the Roles enum is referenced by the production code.

#pragma once

#include <QObject>

namespace dock {

class TaskManager : public QObject
{
    Q_OBJECT
public:
    enum Roles {
        WinIdRole = Qt::UserRole + 1,
        PidRole,
        IdentityRole,
        WinIconRole,
        WinTitleRole,
        ActiveRole,
        ShouldSkipRole,
        AttentionRole,
        ItemIdRole,
        MenusRole,
        WindowsRole,
        DesktopIdRole = 0x1000,
        NameRole,
        IconNameRole,
        StartUpWMClassRole,
        NoDisplayRole,
        ActionsRole,
        DDECategoryRole,
        InstalledTimeRole,
        LastLaunchedTimeRole,
        LaunchedTimesRole,
        DockedRole,
        OnDesktopRole,
        AutoStartRole,
        AppTypeRole,
        XLingLongRole,
        IdRole,
        XCreatedByRole,
        ExecsRole,
        CategoriesRole,
        DesktopSourcePathRole,
    };
    Q_ENUM(Roles)
};

} // namespace dock
