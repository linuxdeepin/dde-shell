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
        {TraySortOrderModel::VisibilityRole, QByteArrayLiteral("visibility")},
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
    connect(this, &TraySortOrderModel::actionsAlwaysVisibleChanged, this, [this](){
        qDebug() << "actionsAlwaysVisibleChanged";
        updateVisualIndexes();
    });
    updateVisualIndexes();
}

TraySortOrderModel::~TraySortOrderModel()
{
    // dtor
}

bool TraySortOrderModel::dropToStashTray(const QString &draggedSurfaceId, int dropVisualIndex, bool isBefore)
{
    // Check if the dragged tray surfaceId exists. Reject if not the case
    QList<QStandardItem *> draggedItems = findItems(draggedSurfaceId);
    if (draggedItems.isEmpty()) return false;
    Q_ASSERT(draggedItems.count() == 1);
    QStringList * sourceSection = getSection(draggedItems[0]->data(SectionTypeRole).toString());

    // drag inside the stashed section
    if (sourceSection == &m_stashedIds) {
        return false;
    } else {
        sourceSection->removeOne(draggedSurfaceId);
        m_stashedIds.append(draggedSurfaceId);
        updateVisualIndexes();
        return true;
    }
}

// drop existing item to tray, return true if drop attempt is accepted, false if rejected
bool TraySortOrderModel::dropToDockTray(const QString &draggedSurfaceId, int dropVisualIndex, bool isBefore)
{
    // Check if the dragged tray surfaceId exists. Reject if not the case
    QList<QStandardItem *> draggedItems = findItems(draggedSurfaceId);
    if (draggedItems.isEmpty()) return false;
    Q_ASSERT(draggedItems.count() == 1);
    QStringList * sourceSection = getSection(draggedItems[0]->data(SectionTypeRole).toString());

    // Find the item attempted to drop on
    QStandardItem * dropOnItem = findItemByVisualIndex(dropVisualIndex, DockTraySection);
    Q_CHECK_PTR(dropOnItem);
    if (!dropOnItem) return false;
    QString dropOnSurfaceId(dropOnItem->data(SurfaceIdRole).toString());

    if (dropOnSurfaceId == QLatin1String("internal/action-show-stash")) {
        if (isBefore) {
            // show stash action is always the first action, cannot drop before it
            return false;
        }
        if (sourceSection == &m_collapsableIds) {
            // same-section move
            m_collapsableIds.move(m_collapsableIds.indexOf(draggedSurfaceId), 0);
        } else {
            // prepend to collapsable section
            sourceSection->removeOne(draggedSurfaceId);
            m_collapsableIds.prepend(draggedSurfaceId);
        }
        updateVisualIndexes();
        return true;
    }

    if (dropOnSurfaceId == QLatin1String("internal/action-toggle-collapse")) {
        QStringList * targetSection = isBefore ? &m_collapsableIds : &m_pinnedIds;
        if (isBefore) {
            // move to the end of collapsable section
            if (targetSection == sourceSection) {
                m_collapsableIds.move(m_collapsableIds.indexOf(draggedSurfaceId), m_collapsableIds.count() - 1);
            } else {
                sourceSection->removeOne(draggedSurfaceId);
                m_collapsableIds.append(draggedSurfaceId);
            }
        } else {
            // move to the beginning of pinned section
            if (targetSection == sourceSection) {
                m_pinnedIds.move(m_pinnedIds.indexOf(draggedSurfaceId), 0);
            } else {
                sourceSection->removeOne(draggedSurfaceId);
                m_pinnedIds.prepend(draggedSurfaceId);
            }
        }
        updateVisualIndexes();
        return true;
    }

    if (dropOnSurfaceId == draggedSurfaceId) {
        // same item, don't need to do anything
        return false;
    }

    QStringList * targetSection = getSection(dropOnItem->data(SectionTypeRole).toString());
    if (targetSection == sourceSection) {
        int sourceIndex = targetSection->indexOf(draggedSurfaceId);
        int targetIndex = targetSection->indexOf(dropOnSurfaceId);
        if (isBefore) targetIndex--;
        if (targetIndex < 0) targetIndex = 0;
        if (sourceIndex == targetIndex) {
            // same item (draggedSurfaceId != dropOnSurfaceId caused by isBefore).
            return false;
        }
        targetSection->move(sourceIndex, targetIndex);
    } else {
        int targetIndex = targetSection->indexOf(dropOnSurfaceId);
        if (!isBefore) targetIndex++;
        sourceSection->removeOne(draggedSurfaceId);
        targetSection->insert(targetIndex, draggedSurfaceId);
    }
    updateVisualIndexes();
    return true;
}

QStandardItem *TraySortOrderModel::findItemByVisualIndex(int visualIndex, VisualSections visualSection) const
{
    QStandardItem * result = nullptr;
    const QModelIndexList matched = match(index(0, 0), VisualIndexRole, visualIndex, -1);
    for (const QModelIndex & index : matched) {
        QString section(data(index, SectionTypeRole).toString());
        if (visualSection == DockTraySection) {
            if (section == QLatin1String("stashed")) continue;
            if (m_collapsed && section == QLatin1String("collapsable")) continue;
        } else {
            if (section != QLatin1String("stashed")) continue;
        }
        if (!data(index, VisibilityRole).toBool()) continue;

        result = itemFromIndex(index);
        break;
    }
    return result;
}

QStringList *TraySortOrderModel::getSection(const QString &sectionType)
{
    if (sectionType == QLatin1String("pinned")) {
        return &m_pinnedIds;
    } else if (sectionType == QLatin1String("collapsable")) {
        return &m_collapsableIds;
    } else if (sectionType == QLatin1String("stashed")) {
        return &m_stashedIds;
    }

    return nullptr;
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
    // stashed action
    // the visual index of stashed items are also for their sort order, but the index
    // number is independently from these non-stashed items.
    int stashedVisualIndex = 0;
    bool showStashActionVisible = m_actionsAlwaysVisible;
    for (const QString & id : std::as_const(m_stashedIds)) {
        QList<QStandardItem *> results = findItems(id);
        if (results.isEmpty()) continue;
        showStashActionVisible = true;
        results[0]->setData(QLatin1String("stashed"), TraySortOrderModel::SectionTypeRole);
        results[0]->setData(stashedVisualIndex, TraySortOrderModel::VisualIndexRole);
        stashedVisualIndex++;
    }

    int currentVisualIndex = 0;
    // "internal/action-show-stash"
    QList<QStandardItem *> results = findItems("internal/action-show-stash");
    Q_ASSERT(!results.isEmpty());
    results[0]->setData(showStashActionVisible, TraySortOrderModel::VisibilityRole);
    if (showStashActionVisible) {
        results[0]->setData(currentVisualIndex, TraySortOrderModel::VisualIndexRole);
        currentVisualIndex++;
    }

    // collapsable
    bool toogleCollapseActionVisible = m_actionsAlwaysVisible;
    for (const QString & id : std::as_const(m_collapsableIds)) {
        QList<QStandardItem *> results = findItems(id);
        if (results.isEmpty()) continue;
        if (!results[0]->data(TraySortOrderModel::VisibilityRole).toBool()) continue;
        toogleCollapseActionVisible = true;
        results[0]->setData(QLatin1String("collapsable"), TraySortOrderModel::SectionTypeRole);
        results[0]->setData(currentVisualIndex, TraySortOrderModel::VisualIndexRole);

        if (!m_collapsed) {
            currentVisualIndex++;
        }
    }

    // "internal/action-toggle-collapse"
    results = findItems("internal/action-toggle-collapse");
    Q_ASSERT(!results.isEmpty());
    results[0]->setData(toogleCollapseActionVisible, TraySortOrderModel::VisibilityRole);
    if (toogleCollapseActionVisible) {
        results[0]->setData(currentVisualIndex, TraySortOrderModel::VisualIndexRole);
        currentVisualIndex++;
    }

    // pinned
    for (const QString & id : std::as_const(m_pinnedIds)) {
        QList<QStandardItem *> results = findItems(id);
        if (results.isEmpty()) continue;
        results[0]->setData(QLatin1String("pinned"), TraySortOrderModel::SectionTypeRole);
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
