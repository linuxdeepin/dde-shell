// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dataaccessorproxy.h"
#include "memoryaccessor.h"

#include <QDebug>

namespace notification
{

DataAccessorProxy *DataAccessorProxy::instance()
{
    static DataAccessorProxy *gInstance = nullptr;
    if (!gInstance) {
        gInstance = new DataAccessorProxy();
        gInstance->m_impl = new MemoryAccessor();
        gInstance->m_source = new MemoryAccessor();
    }
    return gInstance;
}

DataAccessorProxy::~DataAccessorProxy()
{
    if (m_impl) {
        delete m_impl;
        m_impl = nullptr;
    }
}

void DataAccessorProxy::setSource(DataAccessor *source)
{
    if (!source)
        return;

    if (m_source) {
        delete m_source;
    }
    m_source = source;
}

qint64 DataAccessorProxy::addEntity(const NotifyEntity &entity)
{
    if (entity.processedType() == NotifyEntity::NotProcessed) {
        return m_impl->addEntity(entity);
    } else {
        if (!filterToSource(entity)) {
            return m_source->addEntity(entity);
        }
    }
    return -1;
}

qint64 DataAccessorProxy::replaceEntity(qint64 id, const NotifyEntity &entity)
{
    if (entity.processedType() == NotifyEntity::NotProcessed) {
        return m_impl->replaceEntity(id, entity);
    } else {
        return m_source->replaceEntity(id, entity);
    }
}

void DataAccessorProxy::updateEntityProcessedType(qint64 id, int processedType)
{
    if (routerToSource(id, processedType)) {
        m_impl->updateEntityProcessedType(id, processedType);
        auto entity = m_impl->fetchEntity(id);
        if (!filterToSource(entity)) {
            const auto sId = m_source->addEntity(entity);
            if (sId > 0) {
                m_impl->removeEntity(id);
                entity.setId(sId);
            }
        } else {
            m_impl->removeEntity(id);
        }
    } else {
        return m_source->updateEntityProcessedType(id, processedType);
    }
}

NotifyEntity DataAccessorProxy::fetchEntity(qint64 id)
{
    auto entity = m_impl->fetchEntity(id);
    if (entity.isValid())
        return entity;

    return m_source->fetchEntity(id);
}

int DataAccessorProxy::fetchEntityCount(const QString &appName, int processedType) const
{
    if (processedType == NotifyEntity::NotProcessed) {
        return m_impl->fetchEntityCount(appName, processedType);
    } else {
        return m_source->fetchEntityCount(appName, processedType);
    }
}

NotifyEntity DataAccessorProxy::fetchLastEntity(const QString &appName, int processedType)
{
    if (processedType == NotifyEntity::NotProcessed) {
        return m_impl->fetchLastEntity(appName, processedType);
    } else {
        return m_source->fetchLastEntity(appName, processedType);
    }
}

NotifyEntity DataAccessorProxy::fetchLastEntity(uint notifyId)
{
    auto entity = m_impl->fetchLastEntity(notifyId);
    if (entity.isValid())
        return entity;
    return {};
}

QList<NotifyEntity> DataAccessorProxy::fetchEntities(const QString &appName, int processedType, int maxCount)
{
    if (processedType == NotifyEntity::NotProcessed) {
        return m_impl->fetchEntities(appName, processedType, maxCount);
    }

    return m_source->fetchEntities(appName, processedType, maxCount);
}

QList<QString> DataAccessorProxy::fetchApps(int maxCount) const
{
    return m_source->fetchApps(maxCount);
}

void DataAccessorProxy::removeEntity(qint64 id)
{
    if (m_impl->fetchEntity(id).isValid()) {
        m_impl->removeEntity(id);
    } else {
        m_source->removeEntity(id);
    }
}

void DataAccessorProxy::removeEntityByApp(const QString &appName)
{
    m_source->removeEntityByApp(appName);
}

void DataAccessorProxy::removeEntitiesByExpiredTime(qint64 expiredTime)
{
    m_source->removeEntitiesByExpiredTime(expiredTime);
}

void DataAccessorProxy::clear()
{
    m_source->clear();
}

bool DataAccessorProxy::routerToSource(qint64 id, int processedType) const
{
    if (processedType == NotifyEntity::Processed || processedType == NotifyEntity::Removed) {
        auto entity = m_impl->fetchEntity(id);
        return entity.isValid();
    }
    return false;
}

bool DataAccessorProxy::filterToSource(const NotifyEntity &entity) const
{
    // "cancel"表示正在发送蓝牙文件,不需要发送到通知中心
    const auto bluetooth = entity.body().contains("%") && entity.actions().contains("cancel");
    return bluetooth;
}
}
