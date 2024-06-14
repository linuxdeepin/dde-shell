// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "traysortordermodel.h"

#include <QDebug>

namespace docktray {

QStandardItem * TraySortOrderModel::createTrayItem(const QString & name, const QString & sectionType, const QString & delegateType)
{
    QStandardItem * item = new QStandardItem(name);
    item->setData(name, TraySortOrderModel::SurfaceIdRole);
    item->setData(true, TraySortOrderModel::VisibilityRole);
    item->setData(sectionType, TraySortOrderModel::SectionTypeRole);
    item->setData(delegateType, TraySortOrderModel::DelegateTypeRole);

    if (sectionType == "stashed") {
        m_stashedIds.append(name);
    } else if (sectionType == "collapsable") {
        m_collapsableIds.append(name);
    } else if (sectionType == "pinned") {
        m_pinnedIds.append(name);
    }
    qDebug() << sectionType << name;

    return item;
}

TraySortOrderModel::TraySortOrderModel(QObject *parent)
    : QStandardItemModel(parent)
{
    QHash<int, QByteArray> defaultRoleNames = roleNames();
    defaultRoleNames.insert({
        {TraySortOrderModel::SurfaceIdRole, QByteArrayLiteral("surfaceId")},
        {TraySortOrderModel::VisibilityRole, QByteArrayLiteral("visible")},
        {TraySortOrderModel::SectionTypeRole, QByteArrayLiteral("sectionType")},
        {TraySortOrderModel::VisualIndexRole, QByteArrayLiteral("visualIndex")},
        {TraySortOrderModel::DelegateTypeRole, QByteArrayLiteral("delegateType")}
    });
    setItemRoleNames(defaultRoleNames);

    // internal tray actions
    appendRow(createTrayItem("internal/action-show-stash", "tray-action", "action-show-stash"));
    appendRow(createTrayItem("internal/action-toggle-collapse", "tray-action", "action-toggle-collapse"));

    // testing purpose dummy entries.
    appendRow(createTrayItem("web-browser", "stashed", "dummy"));
    appendRow(createTrayItem("folder-trash", "stashed", "dummy"));
    appendRow(createTrayItem("web-browser-symbolic", "collapsable", "dummy"));
    appendRow(createTrayItem("user-info-symbolic", "collapsable", "dummy"));
    appendRow(createTrayItem("folder-symbolic", "pinned", "dummy"));

    connect(this, &TraySortOrderModel::availableSurfacesChanged, this, &TraySortOrderModel::onAvailableSurfacesChanged);

    connect(this, &TraySortOrderModel::collapsedChanged, this, [this](){
        qDebug() << "collapsedChanged";
        updateVisualIndexes();
    });
    updateVisualIndexes();
}

TraySortOrderModel::~TraySortOrderModel()
{
    // dtor
}

void TraySortOrderModel::updateVisualIndexes()
{
    int currentVisualIndex = 0;

    // stashed action
    bool showStashActionVisible = false;
    for (const QString & id : std::as_const(m_stashedIds)) {
        QList<QStandardItem *> results = findItems(id);
        if (results.isEmpty()) continue;
        showStashActionVisible = true;
        break;
    }
    if (showStashActionVisible) {
        QList<QStandardItem *> results = findItems("internal/action-show-stash");
        Q_ASSERT(!results.isEmpty());
        results[0]->setData(currentVisualIndex, TraySortOrderModel::VisualIndexRole);
        currentVisualIndex++;
    }

    // collapsable
    for (const QString & id : std::as_const(m_collapsableIds)) {
        QList<QStandardItem *> results = findItems(id);
        if (results.isEmpty()) continue;
        results[0]->setData(currentVisualIndex, TraySortOrderModel::VisualIndexRole);

        if (!m_collapsed) {
            currentVisualIndex++;
        }
    }

    // "internal/action-toggle-collapse"
    QList<QStandardItem *> results = findItems("internal/action-toggle-collapse");
    Q_ASSERT(!results.isEmpty());
    results[0]->setData(currentVisualIndex, TraySortOrderModel::VisualIndexRole);
    currentVisualIndex++;

    // pinned
    for (const QString & id : std::as_const(m_pinnedIds)) {
        QList<QStandardItem *> results = findItems(id);
        if (results.isEmpty()) continue;
        results[0]->setData(currentVisualIndex, TraySortOrderModel::VisualIndexRole);
        currentVisualIndex++;
    }

    // we don't care about the fixed items here since its width/height is not a fixed value

    // update visible item count property
    setProperty("visualItemCount", currentVisualIndex);

    qDebug() << "update" << m_visualItemCount << currentVisualIndex;
}

void TraySortOrderModel::registerSurfaceId(const QString &name, const QString &delegateType)
{
    QList<QStandardItem *> results = findItems(name);
    if (!results.isEmpty()) {
        return;
    }

    appendRow(createTrayItem(name, "collapsable", delegateType));
}

void TraySortOrderModel::onAvailableSurfacesChanged()
{
    QStringList availableSurfaceIds;
    // check if there is any new tray item needs to be registered, and register it.
    for (const QVariantMap & surface : m_availableSurfaces) {
        // already registered items will be skipped so this is fine
        QString surfaceId(surface.value("surfaceId").toString());
        QString delegateType(surface.value("delegateType", "legacy-tray-plugin").toString());
        registerSurfaceId(surfaceId, delegateType);
        availableSurfaceIds << surfaceId;
    }

    // check if there are tray items no longer availabled, and remove them.
    for (int i = rowCount() - 1; i >= 0; i--) {
        const QString surfaceId(data(index(i, 0), TraySortOrderModel::SurfaceIdRole).toString());
        if (availableSurfaceIds.contains(surfaceId)) continue;
        if (surfaceId.startsWith("internal/")) continue;
        removeRow(i);
    }
    // finally, update visual index
    updateVisualIndexes();
}

}
