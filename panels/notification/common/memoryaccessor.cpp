// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "memoryaccessor.h"
#include <QDebug>
#include <limits>

namespace notification
{

MemoryAccessor::~MemoryAccessor()
{
}

qint64 MemoryAccessor::addEntity(const NotifyEntity &entity)
{
    QMutexLocker locker(&m_mutex);
    m_entities << entity;
    return entity.bubbleId();
}

qint64 MemoryAccessor::replaceEntity(qint64 id, const NotifyEntity &entity)
{
    QMutexLocker locker(&m_mutex);
    auto iter = std::find_if(m_entities.begin(), m_entities.end(), [id](const NotifyEntity &entity) {
        if (entity.id() == id)
            return true;
        return false;
    });

    if (iter != m_entities.end()) {
        const auto index = (iter - m_entities.begin());
        Q_ASSERT(index >= 0);
        m_entities[index] = entity;
    } else {
        return -1;
    }

    return id;
}

void MemoryAccessor::updateEntityProcessedType(qint64 id, int processedType)
{
    QMutexLocker locker(&m_mutex);
    auto iter = std::find_if(m_entities.begin(), m_entities.end(), [id](const NotifyEntity &entity) {
        if (entity.id() == id)
            return true;
        return false;
    });
    if (iter != m_entities.end()) {
        iter->setProcessedType(processedType);
    }
}

NotifyEntity MemoryAccessor::fetchEntity(qint64 id)
{
    QMutexLocker locker(&m_mutex);
    auto iter = std::find_if(m_entities.begin(), m_entities.end(), [id](const NotifyEntity &entity) {
        if (entity.id() == id)
            return true;
        return false;
    });
    if (iter != m_entities.end())
        return *iter;
    return {};
}

int MemoryAccessor::fetchEntityCount(const QString &appName, int processedType) const
{
    QMutexLocker locker(&m_mutex);
    const auto count = std::count_if(m_entities.begin(), m_entities.end(), [appName, processedType](const NotifyEntity &entity) {
        if ((entity.appName() == appName || AllApp() == appName) && entity.processedType() == processedType)
            return true;
        return false;
    });
    return count;
}

NotifyEntity MemoryAccessor::fetchLastEntity(const QString &appName, int processedType)
{
    QMutexLocker locker(&m_mutex);
    auto iter = std::find_if(m_entities.rbegin(), m_entities.rend(), [appName, processedType](const NotifyEntity &entity) {
        if (entity.appName() == appName && entity.processedType() == processedType)
            return true;
        return false;
    });
    if (iter != m_entities.rend())
        return *iter;
    return {};
}

NotifyEntity MemoryAccessor::fetchLastEntity(uint notifyId)
{
    QMutexLocker locker(&m_mutex);
    auto iter = std::find_if(m_entities.rbegin(), m_entities.rend(), [notifyId](const NotifyEntity &entity) {
        if (entity.bubbleId() == notifyId)
            return true;
        return false;
    });
    if (iter != m_entities.rend())
        return *iter;
    return {};
}

QList<NotifyEntity> MemoryAccessor::fetchExpiredEntities(qint64 expiredTime)
{
    QMutexLocker locker(&m_mutex);
    QList<NotifyEntity> expiredEntities;
    
    for (const auto &entity : m_entities) {
        if (entity.cTime() < expiredTime) {
            expiredEntities.append(entity);
        }
    }
    
    std::sort(expiredEntities.begin(), expiredEntities.end(), [](const NotifyEntity &a, const NotifyEntity &b) {
        return a.cTime() < b.cTime();
    });
    
    return expiredEntities;
}

QList<NotifyEntity> MemoryAccessor::fetchEntities(const QString &appName, int processedType, int maxCount)
{
    QMutexLocker locker(&m_mutex);
    QList<NotifyEntity> ret;
    for (const auto &item : m_entities) {
        if (maxCount >= 0 && ret.count() > maxCount)
            break;
        if ((item.appName() == appName || AllApp() == appName) && item.processedType() == processedType) {
            ret.append(item);
        }
    }
    return ret;
}

QList<QString> MemoryAccessor::fetchApps(int maxCount) const
{
    QMutexLocker locker(&m_mutex);
    QList<QString> ret;
    for (const auto &item : m_entities) {
        if (!ret.contains(item.appName())) {
            ret.append(item.appName());
        }
        if (maxCount >= 0 && ret.count() > maxCount)
            break;
    }
    return ret;
}

void MemoryAccessor::removeEntity(qint64 id)
{
    QMutexLocker locker(&m_mutex);
    m_entities.removeIf([id](const NotifyEntity &entity) {
        return entity.id() == id;
    });
}

void MemoryAccessor::removeEntityByApp(const QString &appName)
{
    QMutexLocker locker(&m_mutex);
    m_entities.removeIf([appName](const NotifyEntity &entity) {
        return entity.appName() == appName;
    });
}

void MemoryAccessor::clear()
{
    QMutexLocker locker(&m_mutex);
    m_entities.clear();
}

}
