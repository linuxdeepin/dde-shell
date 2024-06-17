// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "traysortordermodel.h"

#include <QDebug>

namespace docktray {

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

#ifdef QT_DEBUG
    // testing purpose dummy entries.
    appendRow(createTrayItem("trash::trash", "stashed", "dummy"));
    appendRow(createTrayItem("dnd-mode::dnd-mode-key", "stashed", "dummy"));
    appendRow(createTrayItem("web-browser-symbolic", "collapsable", "dummy"));
    appendRow(createTrayItem("user-info-symbolic", "collapsable", "dummy"));
    appendRow(createTrayItem("folder-symbolic", "pinned", "dummy"));
#endif

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

QString TraySortOrderModel::findSection(const QString & surfaceId, const QString & fallback)
{
    if (m_pinnedIds.contains(surfaceId)) return QLatin1String("pinned");
    if (m_collapsableIds.contains(surfaceId)) return QLatin1String("collapsable");
    if (m_stashedIds.contains(surfaceId)) return QLatin1String("stashed");

    return fallback;
}

void TraySortOrderModel::registerToSection(const QString & surfaceId, const QString & sectionType)
{
    QStringList * section = nullptr;
    if (sectionType == "stashed") {
        section = &m_stashedIds;
    } else if (sectionType == "collapsable") {
        section = &m_collapsableIds;
    } else if (sectionType == "pinned") {
        section = &m_pinnedIds;
    } else if (sectionType == "tray-action") {
        // can safely ignore, no need to register to any section
        return;
    }

    if (!section->contains(surfaceId)) {
        section->append(surfaceId);
    }
}

QStandardItem * TraySortOrderModel::createTrayItem(const QString & name, const QString & sectionType, const QString & delegateType)
{
    QString actualSectionType = findSection(name, sectionType);
    registerToSection(name, actualSectionType);

    qDebug() << actualSectionType << name << delegateType;

    QStandardItem * item = new QStandardItem(name);
    item->setData(name, TraySortOrderModel::SurfaceIdRole);
    item->setData(true, TraySortOrderModel::VisibilityRole);
    item->setData(actualSectionType, TraySortOrderModel::SectionTypeRole);
    item->setData(delegateType, TraySortOrderModel::DelegateTypeRole);

    return item;
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
