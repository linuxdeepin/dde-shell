// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "traysortordermodel.h"

#include <QDebug>
#include <DConfig>

namespace docktray {

const QString SECTION_TRAY_ACTION = QLatin1String("tray-action");
const QString SECTION_STASHED = QLatin1String("stashed");
const QString SECTION_COLLAPSABLE = QLatin1String("collapsable");
const QString SECTION_FIXED = QLatin1String("fixed");
const QString SECTION_PINNED = QLatin1String("pinned");

const QString INTERNAL_PREFIX = QLatin1String("internal/");
const QString ACTION_STASH_PLACEHOLDER = QLatin1String("action-stash-placeholder");
const QString ACTION_STASH_PLACEHOLDER_NAME = INTERNAL_PREFIX + ACTION_STASH_PLACEHOLDER;
const QString ACTION_SHOW_STASH = QLatin1String("action-show-stash");
const QString ACTION_SHOW_STASH_NAME = INTERNAL_PREFIX + ACTION_SHOW_STASH;
const QString ACTION_TOGGLE_COLLAPSE = QLatin1String("action-toggle-collapse");
const QString ACTION_TOGGLE_COLLAPSE_NAME = INTERNAL_PREFIX + ACTION_TOGGLE_COLLAPSE;
const QString ACTION_TOGGLE_QUICK_SETTINGS = QLatin1String("action-toggle-quick-settings");
const QString ACTION_TOGGLE_QUICK_SETTINGS_NAME = INTERNAL_PREFIX + ACTION_TOGGLE_QUICK_SETTINGS;

TraySortOrderModel::TraySortOrderModel(QObject *parent)
    : QAbstractItemModel(parent)
    , m_defaultRoleNames({
        {TraySortOrderModel::SurfaceIdRole, QByteArrayLiteral("surfaceId")},
        {TraySortOrderModel::VisibilityRole, QByteArrayLiteral("visibility")},
        {TraySortOrderModel::SectionTypeRole, QByteArrayLiteral("sectionType")},
        {TraySortOrderModel::DelegateTypeRole, QByteArrayLiteral("delegateType")},
        {TraySortOrderModel::ForbiddenSectionsRole, QByteArrayLiteral("forbiddenSections")}
    })
    , m_dconfig(Dtk::Core::DConfig::create("org.deepin.dde.shell", "org.deepin.ds.dock.tray"))
{
    connect(this, &QAbstractListModel::rowsInserted, this, &TraySortOrderModel::rowCountChanged);
    connect(this, &QAbstractListModel::rowsRemoved, this, &TraySortOrderModel::rowCountChanged);

    // init sort order data and hidden list data
    loadDataFromDConfig();

    // internal tray actions
    createTrayItem(ACTION_STASH_PLACEHOLDER_NAME, SECTION_STASHED, ACTION_STASH_PLACEHOLDER);
    createTrayItem(ACTION_SHOW_STASH_NAME, SECTION_TRAY_ACTION, ACTION_SHOW_STASH);
    createTrayItem(ACTION_TOGGLE_COLLAPSE_NAME, SECTION_TRAY_ACTION, ACTION_TOGGLE_COLLAPSE);
    createTrayItem(ACTION_TOGGLE_QUICK_SETTINGS_NAME, SECTION_TRAY_ACTION, ACTION_TOGGLE_QUICK_SETTINGS);

    connect(m_dconfig.get(), &Dtk::Core::DConfig::valueChanged, this, [this](const QString &key){
        if (key == QLatin1String("hiddenSurfaceIds")) {
            loadDataFromDConfig();
            updateVisibilities();
        }
    });

    connect(this, &TraySortOrderModel::availableSurfacesChanged, this, &TraySortOrderModel::onAvailableSurfacesChanged);

    connect(this, &TraySortOrderModel::collapsedChanged, this, [this](){
        qDebug() << "collapsedChanged";
        saveDataToDConfig();
        updateVisibilities();
    });
    connect(this, &TraySortOrderModel::actionsAlwaysVisibleChanged, this, [this](){
        qDebug() << "actionsAlwaysVisibleChanged";
        updateShowStashActionVisible();
    });
}

TraySortOrderModel::~TraySortOrderModel()
{
    // dtor
}

QHash<int, QByteArray> TraySortOrderModel::roleNames() const
{
    return m_defaultRoleNames;
}

QVariant TraySortOrderModel::data(const QModelIndex &index, int role) const
{
    switch (role) {
    case SurfaceIdRole:
        return m_list[index.row()].surfaceId;
    case VisibilityRole:
        return m_list[index.row()].visible;
    case SectionTypeRole:
        return m_list[index.row()].sectionType;
    case DelegateTypeRole:
        return m_list[index.row()].delegateType;
    case ForbiddenSectionsRole:
        return m_list[index.row()].forbiddenSections;
    default:
        return {};
    }
}

QModelIndex TraySortOrderModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid()) return QModelIndex();
    if (row < 0 || row >= m_list.size()) return QModelIndex();
    if (column < 0 || column > 1) return QModelIndex();

    return createIndex(row, column);
}

QModelIndex TraySortOrderModel::parent(const QModelIndex &index) const
{
    return QModelIndex();
}

int TraySortOrderModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;

    return m_list.size();
}

int TraySortOrderModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;

    return 1;
}

// drop existing item to tray, return true if drop attempt is accepted, false if rejected
bool TraySortOrderModel::move(const QString &draggedSurfaceId, const QString &dropOnSurfaceId, bool isBefore)
{
    bool res = false;
    QString sectionType;
    std::tie(res, sectionType) = moveAux(draggedSurfaceId, dropOnSurfaceId, isBefore);
    if (!res) {
        return false;
    }

    auto iter = findItem(draggedSurfaceId);
    Q_ASSERT(iter != m_list.end());

    QVector<Item>::const_iterator insertIter = findInsertionPos(iter->surfaceId, sectionType);
    int sourceRow = std::distance(m_list.begin(), iter);
    int destRow = std::distance(m_list.cbegin(), insertIter);
    if (sourceRow < destRow) {
        destRow--;
    }
    if (sourceRow == destRow) {
        return false;
    }

    QString originSectionType = iter->sectionType;
    if (originSectionType != sectionType) {
        iter->sectionType = sectionType;
        dataChanged(index(sourceRow, 0), index(sourceRow, 0), { SectionTypeRole });
    }

    beginMoveRows(QModelIndex(), sourceRow, sourceRow, QModelIndex(), sourceRow < destRow ? destRow + 1 : destRow);
    m_list.move(sourceRow, destRow);
    endMoveRows();

    if (originSectionType == SECTION_STASHED && sectionType != SECTION_STASHED) {
        m_visibleStashed--;
        updateStashPlaceholderVisible();
    } else if (originSectionType != SECTION_STASHED && sectionType == SECTION_STASHED) {
        m_visibleStashed++;
        updateStashPlaceholderVisible();
    }

    return true;
}

std::tuple<bool, QString> TraySortOrderModel::moveAux(const QString &draggedSurfaceId, const QString &dropOnSurfaceId, bool isBefore)
{
    Q_ASSERT(!m_hiddenIds.contains(draggedSurfaceId));

    if (draggedSurfaceId == dropOnSurfaceId) {
        // same item, don't need to do anything
        return {false, ""};
    }

    // Check if the dragged tray surfaceId exists. Reject if not the case
    auto draggedItemIter = findItem(draggedSurfaceId);
    if (draggedItemIter == m_list.end()) return {false, ""};
    QStringList *sourceSection = getSection(draggedItemIter->sectionType);
    QStringList forbiddenSections(draggedItemIter->forbiddenSections);

    auto dropOnItemIter = findItem(dropOnSurfaceId);
    if (dropOnItemIter == m_list.end()) return {false, ""};

    // Ensure position adjustment will be saved at last
    auto deferSaveSortOrder = qScopeGuard([this](){saveDataToDConfig();});

    if (dropOnSurfaceId == ACTION_STASH_PLACEHOLDER_NAME) {
        // placeholder 只能是 stash 中最后一个
        if (!isBefore) {
            return {false, ""};
        }

        QStringList *targetSection = &m_stashedIds;

        // move to the end of stashed section
        if (targetSection == sourceSection) {
            m_stashedIds.move(m_stashedIds.indexOf(draggedSurfaceId), m_stashedIds.count() - 1);
        } else {
            sourceSection->removeOne(draggedSurfaceId);
            m_stashedIds.append(draggedSurfaceId);
        }
        return {true, SECTION_STASHED};
    }

    if (dropOnSurfaceId == ACTION_SHOW_STASH_NAME) {
        // show stash action is always the first action, drop before it consider as drop into stashed area
        if (sourceSection != &m_stashedIds) {
            sourceSection->removeOne(draggedSurfaceId);
            m_stashedIds.append(draggedSurfaceId);
            return {true, SECTION_STASHED};
        } else {
            // already in the stashed tray
            return {false, ""};
        }
        if (sourceSection == &m_collapsableIds) {
            // same-section move
            m_collapsableIds.move(m_collapsableIds.indexOf(draggedSurfaceId), 0);
        } else {
            // prepend to collapsable section
            sourceSection->removeOne(draggedSurfaceId);
            m_collapsableIds.prepend(draggedSurfaceId);
        }
        return {true, SECTION_COLLAPSABLE};
    }

    if (dropOnSurfaceId == ACTION_TOGGLE_COLLAPSE_NAME) {
        QStringList * targetSection = isBefore ? &m_collapsableIds : &m_pinnedIds;
        if (isBefore) {
            // move to the end of collapsable section
            if (forbiddenSections.contains(SECTION_COLLAPSABLE)) return {false, ""};
            if (targetSection == sourceSection) {
                m_collapsableIds.move(m_collapsableIds.indexOf(draggedSurfaceId), m_collapsableIds.count() - 1);
            } else {
                sourceSection->removeOne(draggedSurfaceId);
                m_collapsableIds.append(draggedSurfaceId);
            }
            return {true, SECTION_COLLAPSABLE};
        } else {
            // move to the beginning of pinned section
            if (forbiddenSections.contains(SECTION_PINNED)) return {false, ""};
            if (targetSection == sourceSection) {
                m_pinnedIds.move(m_pinnedIds.indexOf(draggedSurfaceId), 0);
            } else {
                sourceSection->removeOne(draggedSurfaceId);
                m_pinnedIds.prepend(draggedSurfaceId);
            }
            return {true, SECTION_PINNED};
        }
    }

    if (dropOnSurfaceId == ACTION_TOGGLE_QUICK_SETTINGS_NAME) {
        if (!isBefore) {
            return {false, ""};
        }

        QStringList *targetSection = &m_pinnedIds;

        // move to the end of pinned section
        if (forbiddenSections.contains(SECTION_PINNED)) return {false, ""};
        if (targetSection == sourceSection) {
            m_pinnedIds.move(m_pinnedIds.indexOf(draggedSurfaceId), m_pinnedIds.count() - 1);
        } else {
            sourceSection->removeOne(draggedSurfaceId);
            m_pinnedIds.append(draggedSurfaceId);
        }

        return {true, SECTION_PINNED};
    }

    QString targetSectionName(dropOnItemIter->sectionType);
    if (forbiddenSections.contains(targetSectionName)) return {false, ""};
    QStringList * targetSection = getSection(targetSectionName);
    if (targetSection == sourceSection) {
        int sourceIndex = targetSection->indexOf(draggedSurfaceId);
        int targetIndex = targetSection->indexOf(dropOnSurfaceId);
        if (isBefore && sourceIndex < targetIndex) {
            targetIndex--;
        }
        if (sourceIndex == targetIndex) {
            // same item (draggedSurfaceId != dropOnSurfaceId caused by isBefore).
            return {false, ""};
        }
        targetSection->move(sourceIndex, targetIndex);
    } else {
        int targetIndex = targetSection->indexOf(dropOnSurfaceId);
        if (!isBefore) targetIndex++;
        sourceSection->removeOne(draggedSurfaceId);
        targetSection->insert(targetIndex, draggedSurfaceId);
    }
    return {true, targetSectionName};
}

void TraySortOrderModel::setSurfaceVisible(const QString &surfaceId, bool visible)
{
    if (visible) {
        if (m_hiddenIds.contains(surfaceId)) {
            m_hiddenIds.removeOne(surfaceId);
        }
    } else {
        if (!m_hiddenIds.contains(surfaceId)) {
            m_hiddenIds.append(surfaceId);
        }
    }

    auto i = findItem(surfaceId);
    if (i == m_list.end()) {
        return;
    }

    i->visible = getSurfaceVisible(i->surfaceId, i->sectionType);
    int row = std::distance(m_list.begin(), i);
    auto idx = index(row, 0);
    dataChanged(idx, idx, { VisibilityRole });
}

QVector<Item>::iterator TraySortOrderModel::findItem(const QString &surfaceId)
{
    return std::find_if(m_list.begin(), m_list.end(), [surfaceId](const Item & item) {
        return item.surfaceId == surfaceId;
    });
}

QStringList *TraySortOrderModel::getSection(const QString &sectionType)
{
    if (sectionType == SECTION_PINNED) {
        return &m_pinnedIds;
    } else if (sectionType == SECTION_COLLAPSABLE) {
        return &m_collapsableIds;
    } else if (sectionType == SECTION_STASHED) {
        return &m_stashedIds;
    } else if (sectionType == SECTION_FIXED) {
        return &m_fixedIds;
    }

    return nullptr;
}

QString TraySortOrderModel::findSection(const QString & surfaceId, const QString & fallback, const QStringList & forbiddenSections, bool isForceDock)
{
    QStringList * found = nullptr;
    QString result(fallback);

    if (m_pinnedIds.contains(surfaceId)) {
        found = &m_pinnedIds;
        result = SECTION_PINNED;
    } else if (m_collapsableIds.contains(surfaceId)) {
        found = &m_collapsableIds;
        result = SECTION_COLLAPSABLE;
    } else if (m_stashedIds.contains(surfaceId)) {
        found = &m_stashedIds;
        result = SECTION_STASHED;
    } else if (m_fixedIds.contains(surfaceId)) {
        found = &m_fixedIds;
        result = SECTION_FIXED;
    }

    // 设置默认隐藏
    if (!found &&                   // 不在列表中
        !isForceDock &&             // 非 forceDock
        result != SECTION_FIXED &&  // 非固定位置插件（时间）
        !surfaceId.startsWith(INTERNAL_PREFIX) &&   // 非内置插件
        !surfaceId.startsWith("application-tray::") // 非托盘图标
        ) {
        if (!m_hiddenIds.contains(surfaceId)) {
            m_hiddenIds.append(surfaceId);
        }
    }

    if (forbiddenSections.contains(result)) {
        Q_ASSERT(result != fallback);
        if (found) {
            found->removeOne(surfaceId);
            result = fallback;
        }
    }

    return result;
}

void TraySortOrderModel::registerToSection(const QString & surfaceId, const QString & sectionType)
{
    QStringList * section = getSection(sectionType);

    if (section == nullptr) {
        Q_ASSERT(sectionType == SECTION_TRAY_ACTION);
        return;
    }

    // stash placeholder 永远在 stash 最后，不保存位置
    if (surfaceId == ACTION_STASH_PLACEHOLDER_NAME) {
        return;
    }

    if (!section->contains(surfaceId)) {
        section->prepend(surfaceId);
    }
}

QVector<Item>::const_iterator TraySortOrderModel::findInsertionPosInSection(const QString &surfaceId, const QString &sectionType,
                                                            QVector<Item>::const_iterator sectionBegin, QVector<Item>::const_iterator sectionEnd) const
{
    QStringList ids;
    if (sectionType == SECTION_STASHED) {
        ids = m_stashedIds;
    } else if (sectionType == SECTION_COLLAPSABLE) {
        ids = m_collapsableIds;
    } else if (sectionType == SECTION_PINNED) {
        ids = m_pinnedIds;
    } else if (sectionType == SECTION_FIXED) {
        ids = m_fixedIds;
    }

    auto ri = std::find(ids.cbegin(), ids.cend(), surfaceId);
    if (ri == ids.cend()) {
        // 不在列表中，插入开头
        return sectionBegin;
    }

    // 获取下一个的迭代器
    ri++;
    for (; ri != ids.cend(); ri++) {
        auto res = std::find_if(sectionBegin, sectionEnd, [ri](const Item &item) {
            return item.surfaceId == *ri;
        });
        if (res != sectionEnd) {
            return res;
        }
    }

    // 找不到下一个的迭代器，说明当前是最后一个
    return sectionEnd;
}

QVector<Item>::const_iterator TraySortOrderModel::findInsertionPos(const QString &surfaceId, const QString &sectionType) const
{
    Q_ASSERT(!m_hiddenIds.contains(surfaceId));

    auto stashedSectionBegin = m_list.cbegin();
    auto stashedSectionEnd = std::find_if_not(stashedSectionBegin, m_list.cend(), [](const Item &item) {
        return item.sectionType == SECTION_STASHED && item.surfaceId != ACTION_STASH_PLACEHOLDER_NAME;
    });
    if (sectionType == SECTION_STASHED && surfaceId != ACTION_STASH_PLACEHOLDER_NAME) {
        return findInsertionPosInSection(surfaceId, sectionType, stashedSectionBegin, stashedSectionEnd);
    }

    auto stashPlaceholderActionBegin = stashedSectionEnd;
    auto stashPlaceholderActionEnd = std::find_if_not(stashPlaceholderActionBegin, m_list.cend(), [](const Item &item) {
        return item.surfaceId == ACTION_STASH_PLACEHOLDER_NAME;
    });
    if (surfaceId == ACTION_SHOW_STASH_NAME) {
        return stashPlaceholderActionBegin;
    }

    auto showStashActionBegin = stashedSectionEnd;
    auto showStashActionEnd = std::find_if_not(showStashActionBegin, m_list.cend(), [](const Item &item) {
        return item.surfaceId == ACTION_SHOW_STASH_NAME;
    });
    if (surfaceId == ACTION_SHOW_STASH_NAME) {
        return showStashActionBegin;
    }

    auto collapsableSectionBegin = showStashActionEnd;
    auto collapsableSectionEnd = std::find_if_not(collapsableSectionBegin, m_list.cend(), [](const Item &item) {
        return item.sectionType == SECTION_COLLAPSABLE;
    });
    if (sectionType == SECTION_COLLAPSABLE) {
        Q_ASSERT(m_collapsableIds.contains(surfaceId));
        return findInsertionPosInSection(surfaceId, sectionType, collapsableSectionBegin, collapsableSectionEnd);
    }

    auto toggleCollapseActionBegin = collapsableSectionEnd;
    auto toggleCollapseActionEnd = std::find_if_not(toggleCollapseActionBegin, m_list.cend(), [](const Item &item) {
        return item.surfaceId == ACTION_TOGGLE_COLLAPSE_NAME;
    });
    if (surfaceId == ACTION_TOGGLE_COLLAPSE_NAME) {
        return toggleCollapseActionBegin;
    }

    auto pinnedSectionBegin = toggleCollapseActionEnd;
    auto pinnedSectionEnd = std::find_if_not(pinnedSectionBegin, m_list.cend(), [](const Item &item) {
        return item.sectionType == SECTION_PINNED;
    });
    if (sectionType == SECTION_PINNED) {
        Q_ASSERT(m_pinnedIds.contains(surfaceId));
        return findInsertionPosInSection(surfaceId, sectionType, pinnedSectionBegin, pinnedSectionEnd);
    }

    auto toggleQuickSettingsActionBegin = pinnedSectionEnd;
    auto toggleQuickSettingsActionEnd = std::find_if_not(toggleQuickSettingsActionBegin, m_list.cend(), [](const Item &item) {
        return item.surfaceId == ACTION_TOGGLE_QUICK_SETTINGS_NAME;
    });
    if (surfaceId == ACTION_TOGGLE_QUICK_SETTINGS_NAME) {
        return toggleQuickSettingsActionBegin;
    }

    auto fixedSectionBegin = toggleQuickSettingsActionEnd;
    auto fixedSectionEnd = m_list.cend();
    return findInsertionPosInSection(surfaceId, sectionType, fixedSectionBegin, fixedSectionEnd);
}

void TraySortOrderModel::createTrayItem(const QString & name, const QString & sectionType,
                                        const QString & delegateType, const QStringList &forbiddenSections,
                                        bool isForceDock)
{
    QString actualSectionType = findSection(name, sectionType, forbiddenSections, isForceDock);
    registerToSection(name, actualSectionType);

    qDebug() << "====createTrayItem" << actualSectionType << name << delegateType;
    Item item = {
        name,
        getSurfaceVisible(name, actualSectionType),
        actualSectionType,
        delegateType,
        forbiddenSections,
        isForceDock,
    };

    QVector<Item>::const_iterator insertIter = findInsertionPos(name, actualSectionType);

    int row = std::distance(m_list.cbegin(), insertIter);
    beginInsertRows(QModelIndex(), row, row);
    m_list.insert(insertIter, item);
    endInsertRows();

    if (actualSectionType == SECTION_STASHED && name != ACTION_STASH_PLACEHOLDER_NAME && item.visible) {
        m_visibleStashed++;
        updateStashPlaceholderVisible();
    }
}

QString TraySortOrderModel::registerSurfaceId(const QVariantMap & surfaceData)
{
    QString surfaceId(surfaceData.value("surfaceId").toString());
    QString delegateType(surfaceData.value("delegateType", "legacy-tray-plugin").toString());
    QString preferredSection(surfaceData.value("sectionType", "collapsable").toString());
    QStringList forbiddenSections(surfaceData.value("forbiddenSections").toStringList());
    bool isForceDock(surfaceData.value("isForceDock").toBool());

    auto i = findItem(surfaceId);
    if (i != m_list.end()) {
        // check if the item is currently in a forbidden zone
        QString currentSection(i->sectionType);
        if (!forbiddenSections.contains(currentSection)) {
            return surfaceId;
        }

        int row = std::distance(m_list.begin(), i);
        beginRemoveRows(QModelIndex(), row, row);
        m_list.erase(i);
        endRemoveRows();
    }

    createTrayItem(surfaceId, preferredSection, delegateType, forbiddenSections, isForceDock);
    return surfaceId;
}

bool TraySortOrderModel::getSurfaceVisible(const QString &surfaceId, const QString &sectionType)
{
    if (surfaceId == ACTION_STASH_PLACEHOLDER_NAME) {
        return getStashPlaceholderVisible();
    }

    if (surfaceId == ACTION_SHOW_STASH_NAME) {
        return getShowStashActionVisible();
    }

    if (sectionType == SECTION_TRAY_ACTION) {
        return true;
    }

    if (
        m_hiddenIds.contains(surfaceId) ||
        (m_collapsed && sectionType == SECTION_COLLAPSABLE)
    ) {
        return false;
    }

    return true;
}

bool TraySortOrderModel::getStashPlaceholderVisible()
{
    return m_visibleStashed == 0;
}

bool TraySortOrderModel::getShowStashActionVisible()
{
    return m_actionsAlwaysVisible || m_visibleStashed != 0;
}

void TraySortOrderModel::loadDataFromDConfig()
{
    m_stashedIds = m_dconfig->value("stashedSurfaceIds").toStringList();
    m_collapsableIds = m_dconfig->value("collapsableSurfaceIds").toStringList();
    m_pinnedIds = m_dconfig->value("pinnedSurfaceIds").toStringList();
    m_hiddenIds = m_dconfig->value("hiddenSurfaceIds").toStringList();
    m_collapsed = m_dconfig->value("isCollapsed").toBool();
}

void TraySortOrderModel::saveDataToDConfig()
{
    m_dconfig->setValue("stashedSurfaceIds", m_stashedIds);
    m_dconfig->setValue("collapsableSurfaceIds", m_collapsableIds);
    m_dconfig->setValue("pinnedSurfaceIds", m_pinnedIds);
    m_dconfig->setValue("hiddenSurfaceIds", m_hiddenIds);
    m_dconfig->setValue("isCollapsed", m_collapsed);
}

void TraySortOrderModel::onAvailableSurfacesChanged()
{
    QStringList availableSurfaceIds;
    // check if there is any new tray item needs to be registered, and register it.
    for (const QVariantMap & surface : m_availableSurfaces) {
        // already registered items will be checked so this is fine
        const QString surfaceId(registerSurfaceId(surface));
        availableSurfaceIds << surfaceId;
    }

    // check if there are tray items no longer availabled, and remove them.
    auto i = m_list.begin(); 
    while (i != m_list.end()) {
        if (availableSurfaceIds.contains(i->surfaceId)) {
            ++i;
            continue;
        }
        if (i->surfaceId.startsWith(INTERNAL_PREFIX)) {
            ++i;
            continue;
        }

        int row = std::distance(m_list.begin(), i);
        beginRemoveRows(QModelIndex(), row, row);
        i = m_list.erase(i);
        endRemoveRows();
    }

    updateVisibilities();

    // and also save the current sort order
    saveDataToDConfig();
}

void TraySortOrderModel::updateStashPlaceholderVisible()
{
    auto i = findItem(ACTION_STASH_PLACEHOLDER_NAME);
    if (i == m_list.end()) {
        return;
    }

    bool visible = getStashPlaceholderVisible();
    if (i->visible == visible) {
        return;
    }

    i->visible = visible;
    QModelIndex idx = index(std::distance(m_list.begin(), i), 0);
    dataChanged(idx, idx, { VisibilityRole });
}

void TraySortOrderModel::updateShowStashActionVisible()
{
    auto i = findItem(ACTION_SHOW_STASH_NAME);
    if (i == m_list.end()) {
        return;
    }

    bool visible = getShowStashActionVisible();
    if (i->visible == visible) {
        return;
    }

    i->visible = visible;
    QModelIndex idx = index(std::distance(m_list.begin(), i), 0);
    dataChanged(idx, idx, { VisibilityRole });
}

void TraySortOrderModel::updateVisibilities()
{
    m_visibleStashed = 0;
    for (auto i = m_list.begin(); i != m_list.end(); ++i) {
        bool visible = getSurfaceVisible(i->surfaceId, i->sectionType);
        if (visible && i->sectionType == SECTION_STASHED && i->surfaceId != ACTION_STASH_PLACEHOLDER_NAME) {
            m_visibleStashed++;
        }

        // placeholder 在 stash 最后
        if (i->surfaceId == ACTION_STASH_PLACEHOLDER_NAME) {
            visible = getStashPlaceholderVisible();
        } else if (i->surfaceId == ACTION_SHOW_STASH_NAME) {
            visible = getShowStashActionVisible();
        }

        if (i->visible == visible) {
            continue;
        }

        i->visible = visible;

        QModelIndex idx = index(std::distance(m_list.begin(), i), 0);
        dataChanged(idx, idx, { VisibilityRole });
    }
}

}
