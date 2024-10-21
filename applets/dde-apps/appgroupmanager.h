// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <DConfig>
#include <QHash>
#include <QObject>
#include <QTimer>

#include <QStandardItemModel>
#include <tuple>

namespace apps
{
class AppGroup;
/*! \brief AppGroupManager is a interface to manager all groups.
 *
 *  The life cycle of all groups and the appitems of the group are manager by this class.
 */
class AppGroupManager : public QStandardItemModel
{
    Q_OBJECT

public:
    enum Roles {
        GroupIdRole = Qt::UserRole + 1,
        GroupNameRole,
        GroupAppItemsRole,
        ExtendRole = 0x1000,
    };

    static AppGroupManager *instance();

    QVariant data(const QModelIndex &index, int role = GroupIdRole) const override;

    std::tuple<int, int, int> getAppGroupInfo(const QString &appId);
    void setAppGroupInfo(const QString &appId, std::tuple<int, int, int> groupInfo);

private:
    AppGroupManager(QObject *parent = nullptr);
    void loadAppGroupInfo();
    void dumpAppGroupInfo();

private:
    QHash<QString, std::tuple<int, int>> m_map;
    QTimer *m_dumpTimer;
    Dtk::Core::DConfig *m_config;
};
}
