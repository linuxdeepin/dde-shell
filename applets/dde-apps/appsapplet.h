// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "appitemmodel.h"
#include "applet.h"
#include "dsglobal.h"

#include <QAbstractListModel>

DS_USE_NAMESPACE

namespace apps {
class AppItem;
class AppsApplet : public DApplet
{
    Q_OBJECT
    Q_PROPERTY(QAbstractListModel* appModel READ appModel CONSTANT FINAL)

public:
    explicit AppsApplet(QObject *parent = nullptr);
    ~AppsApplet();

    bool load() override;

    QAbstractListModel* appModel() const;

private:
    AppItemModel* m_model;
};
}
