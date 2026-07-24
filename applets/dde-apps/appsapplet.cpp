// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "appsapplet.h"
#include "amappitem.h"
#include "amappitemmodel.h"
#include "appgroup.h"
#include "appgroupmanager.h"
#include "appitemmodel.h"
#include "itemspage.h"
#include "pluginfactory.h"

#include <DUtil>
#include <QtConcurrent>

namespace apps
{
static AMAppItem *findApplication(AMAppItemModel *model, const QString &desktopId)
{
    if (auto item = model->appItem(desktopId))
        return item;

    const auto alternateId = desktopId.endsWith(QStringLiteral(".desktop"))
        ? desktopId.chopped(8)
        : desktopId + QStringLiteral(".desktop");
    return model->appItem(alternateId);
}

AppsApplet::AppsApplet(QObject *parent)
    : DApplet(parent)
    , m_appModel(new AMAppItemModel(this))
    , m_groupModel(new AppGroupManager(m_appModel, this))
{
    connect(m_appModel, &AMAppItemModel::readyChanged, this, &AppsApplet::appModelReadyChanged);
}

AppsApplet::~AppsApplet()
{

}

bool AppsApplet::load()
{
    return true;
}

QAbstractItemModel *AppsApplet::groupModel() const
{
    return m_groupModel;
}

QAbstractItemModel *AppsApplet::appModel() const
{
    return m_appModel;
}

QVariantList AppsApplet::groupItemDetails(const QString &groupId) const
{
    const int id = AppGroup::parseGroupId(groupId);
    auto group = id > 0 ? m_groupModel->group(id) : nullptr;
    if (!group)
        return {};

    const auto items = group->itemsPage()->allArrangedItems();
    QVariantList result;
    result.reserve(items.size());
    for (const auto &desktopId : items) {
        const auto item = findApplication(m_appModel, desktopId);
        if (!item)
            continue;
        result.append(QVariantMap{
            {QStringLiteral("desktopId"), desktopId},
            {QStringLiteral("name"), item->data(AppItemModel::NameRole)},
            {QStringLiteral("iconName"), item->data(AppItemModel::IconNameRole)},
        });
    }
    return result;
}

bool AppsApplet::launchApplication(const QString &desktopId)
{
    if (auto item = findApplication(m_appModel, desktopId)) {
        item->launch({}, {}, {{QStringLiteral("_launch_type"), QStringLiteral("dde-shell")}});
        return true;
    }

    return false;
}

bool AppsApplet::appModelReady() const
{
    return m_appModel->ready();
}

D_APPLET_CLASS(AppsApplet)
}

#include "appsapplet.moc"
