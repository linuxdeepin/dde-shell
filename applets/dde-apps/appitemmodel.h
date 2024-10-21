// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "appgroupmanager.h"
#include <QStandardItemModel>

namespace apps
{
class AppItemModel : public QStandardItemModel
{
public:
    enum Roles {
        DesktopIdRole = AppGroupManager::ExtendRole,
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
        GroupRole,
    };
    Q_ENUM(Roles)

    // This is different from the menu-spec Main Categories list.
    enum DDECategories {
        Internet, // 网络模式
        Chat, // 社交模式
        Music, // 音乐模式
        Video, // 视频模式
        Graphics, // 图形图像
        Game, //
        Office, // 办公模式
        Reading, // 阅读模式
        Development, // 编程开发模式
        System, // 系统管理模式
        Others,
    };
    Q_ENUM(DDECategories)

    explicit AppItemModel(QObject *parent = nullptr);
    QHash<int, QByteArray> roleNames() const override;
};
}
