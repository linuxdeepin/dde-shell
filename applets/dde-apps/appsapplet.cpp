// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "appsapplet.h"
#include "amappitemmodel.h"
#include "appgroupmanager.h"
#include "appitemmodel.h"
#include "pluginfactory.h"

#include <QMetaEnum>

namespace apps
{
AppsApplet::AppsApplet(QObject *parent)
    : DApplet(parent)
    , m_appModel(new AMAppItemModel(this))
    , m_groupModel(new AppGroupManager(m_appModel, this))
{
    connect(m_appModel, &AMAppItemModel::readyChanged, this, &AppsApplet::appModelReadyChanged);
}

QAbstractItemModel *AppsApplet::groupModel() const
{
    return m_groupModel;
}

QAbstractItemModel *AppsApplet::appModel() const
{
    return m_appModel;
}

bool AppsApplet::appModelReady() const
{
    return m_appModel->ready();
}

QVariantMap AppsApplet::ddeCategories() const
{
    QVariantMap categories;
    const QMetaEnum metaEnum = QMetaEnum::fromType<AppItemModel::DDECategories>();
    for (int i = 0; i < metaEnum.keyCount(); ++i)
        categories.insert(QString::fromLatin1(metaEnum.key(i)), metaEnum.value(i));
    return categories;
}

D_APPLET_CLASS(AppsApplet)
}

#include "appsapplet.moc"
