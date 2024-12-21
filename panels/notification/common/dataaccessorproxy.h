// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QList>
#include <QMutex>
#include <QString>

#include "dataaccessor.h"

namespace notification
{

class DataAccessorProxy : public DataAccessor
{
public:
    static DataAccessorProxy *instance();
    ~DataAccessorProxy() override;

    void setSource(DataAccessor *source);

    virtual qint64 addEntity(const NotifyEntity &entity) override;
    virtual qint64 replaceEntity(qint64 id, const NotifyEntity &entity) override;

    virtual void updateEntityProcessedType(qint64 id, int processedType) override;

    virtual NotifyEntity fetchEntity(qint64 id) override;

    virtual int fetchEntityCount(const QString &appName, int processedType) const override;
    virtual NotifyEntity fetchLastEntity(const QString &appName, int processedType) override;
    virtual NotifyEntity fetchLastEntity(uint notifyId) override;
    virtual QList<NotifyEntity> fetchEntities(const QString &appName, int processedType, int maxCount) override;
    virtual QList<QString> fetchApps(int maxCount) const override;

    virtual void removeEntity(qint64 id) override;
    virtual void removeEntityByApp(const QString &appName) override;
    virtual void clear() override;

private:
    bool routerToSource(qint64 id, int processedType) const;

private:
    DataAccessor *m_source = nullptr;
    DataAccessor *m_impl = nullptr;
};

}
