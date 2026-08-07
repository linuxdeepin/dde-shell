// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "applet.h"
#include "dsglobal.h"

#include <QAbstractItemModel>
#include <QVariantMap>

DS_USE_NAMESPACE

namespace apps {
class AMAppItemModel;
class AppsApplet : public DApplet
{
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel *appModel READ appModel CONSTANT FINAL)
    Q_PROPERTY(bool appModelReady READ appModelReady NOTIFY appModelReadyChanged FINAL)
    Q_PROPERTY(QAbstractItemModel *appGroupModel READ groupModel CONSTANT FINAL)
    Q_PROPERTY(QVariantMap ddeCategories READ ddeCategories CONSTANT FINAL)

public:
    explicit AppsApplet(QObject *parent = nullptr);

    QAbstractItemModel *appModel() const;
    QAbstractItemModel *groupModel() const;

    bool appModelReady() const;
    QVariantMap ddeCategories() const;

signals:
    void appModelReadyChanged(bool ready);

private:
    AMAppItemModel *m_appModel;
    QAbstractItemModel *m_groupModel;
};
}
