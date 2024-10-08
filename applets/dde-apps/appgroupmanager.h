// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QHash>
#include <QObject>
#include <DConfig>
#include <QTimer>

namespace apps {
class AppGroup;
/*! \brief AppGroupManager is a interface to manager all groups.
 *
 *  The life cycle of all groups and the appitems of the group are manager by this class.
 */
class AppGroupManager : public QObject
{
    Q_OBJECT

public:
    uint getAppGroupForAppItemId(const QString &appItemId);
    void setGropForAppItemId(const QString &appItemId, const uint &id, const uint &pos);
    static AppGroupManager* instance();

private:
    void loadeAppGroupInfo();
    void dumpAppGroupInfo();

private:
    AppGroupManager(QObject* parent = nullptr);
    QList<AppGroup*> m_groups;

    Dtk::Core::DConfig* m_config;
    QTimer* m_dumpTimer;
};
}
