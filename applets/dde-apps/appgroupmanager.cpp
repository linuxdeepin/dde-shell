// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "appgroupmanager.h"
#include "appgroup.h"

#include <QtConcurrent>

namespace apps {
AppGroupManager* AppGroupManager::instance()
{
    static AppGroupManager* _instance = nullptr;
    if (_instance == nullptr) {
        _instance = new AppGroupManager;
    }

    return _instance;
}
AppGroupManager::AppGroupManager(QObject* parent)
    : QObject(parent)
    , m_config(Dtk::Core::DConfig::create("org.deepin.dde.shell","org.deepin.ds.dde-apps","", this))
    , m_dumpTimer(new QTimer(this))
{
    m_dumpTimer->setSingleShot(true);
    m_dumpTimer->setInterval(1000);
    connect(m_dumpTimer, &QTimer::timeout, this, [this](){
        dumpAppGroupInfo();
    });

    loadeAppGroupInfo();
}

uint AppGroupManager::getAppGroupForAppItemId(const QString &appItemId)
{
    auto it = std::find_if(m_groups.constBegin(), m_groups.constEnd(), [appItemId](AppGroup* const group){
        return group->appItems().contains(appItemId);
    });

    return it == m_groups.constEnd() ? 0 : (*it)->id();
}

void AppGroupManager::setGropForAppItemId(const QString &appItemId, const uint &id, const uint &pos)
{
    // remove appitemId from origin group
    if (0 == id) {
        auto it = std::find_if(m_groups.begin(), m_groups.end(), [appItemId](AppGroup* const group){
            return group->appItems().contains(appItemId);
        });

        if ( m_groups.end() == it)
            return;

        (*it)->removeItem(appItemId);
        // 尺寸为 0 时而不是1，AppGroup 进行清理
        if ((*it)->appItems().size() == 0) {
            m_groups.removeAt(it - m_groups.begin());
            (*it)->deleteLater();
        }
        return;
    }

    auto it = std::find_if(m_groups.constBegin(), m_groups.constEnd(), [id](AppGroup* const group){
        return group->id() == id;
    });

    if (it == m_groups.constEnd()) {
        m_groups.append(new AppGroup(id, "",{appItemId}, {1}));
    } else {
        (*it)->instertItem(appItemId, pos);
    }
    
    m_dumpTimer->start();
}

void AppGroupManager::loadeAppGroupInfo()
{
    m_groups.clear();
    auto config = m_config->value("Groups").toMap();
    for (auto c : config.keys()) {
        bool ok = false;
        auto id = c.toInt(&ok);
        if (!ok) continue;

        auto groupData = config.value(c).toMap();
        QString groupName = groupData["name"].toString();
        QList<int> pages = groupData["pages"].value<QList<int>>();
        QStringList itemIds = groupData["items"].toStringList();
        m_groups.append(new AppGroup(id, groupName, itemIds, pages));
    }
}

void AppGroupManager::dumpAppGroupInfo()
{
    QVariantMap res;
    for (auto group : m_groups) {
        QVariantMap groupData;
        groupData["name"] = group->name();
        QVariantList pages;
        for (auto p : group->pages()) {
            pages << p;
        }
        groupData["pages"] = pages;
        groupData["items"] = group->appItems();
        res[QString::number(group->id())] = groupData;
    }

    m_config->setValue("Groups", res);
}
}
