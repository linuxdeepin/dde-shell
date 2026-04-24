// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "globals.h"
#include "taskmanagersettings.h"

#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QStandardPaths>

#include <string>

#include <yaml-cpp/yaml.h>

namespace dock {
namespace {
static const QString DOWNLOADS_PLACEHOLDER = QStringLiteral("folder/$DOWNLOADS");
static const QString APPLICATIONS_FOLDER_DOCK_ELEMENT = QStringLiteral("folder//usr/share/applications");
static constexpr int DEFAULT_DOCK_FOLDERS_MIGRATION_VERSION = 1;

static QStringList stockDesktopDockedElements()
{
    return {
        QStringLiteral("desktop/dde-file-manager"),
        QStringLiteral("desktop/deepin-app-store"),
        QStringLiteral("desktop/org.deepin.browser"),
        QStringLiteral("desktop/deepin-mail"),
        QStringLiteral("desktop/deepin-terminal"),
        QStringLiteral("desktop/dde-calendar"),
        QStringLiteral("desktop/deepin-music"),
        QStringLiteral("desktop/deepin-editor"),
        QStringLiteral("desktop/deepin-calculator"),
        QStringLiteral("desktop/org.deepin.dde.control-center"),
    };
}

static QString resolveDockedElement(const QString &element)
{
    if (element != DOWNLOADS_PLACEHOLDER) {
        return element;
    }

    const QString downloadsPath = QDir::cleanPath(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
    if (downloadsPath.isEmpty()) {
        return {};
    }

    return QStringLiteral("folder/%1").arg(downloadsPath);
}

static QStringList resolveDockedElements(const QStringList &elements)
{
    QStringList resolvedElements;

    for (const QString &element : elements) {
        const QString resolvedElement = resolveDockedElement(element);
        if (resolvedElement.isEmpty() || resolvedElements.contains(resolvedElement)) {
            continue;
        }

        resolvedElements.append(resolvedElement);
    }

    return resolvedElements;
}

static QSet<QString> asSet(const QStringList &elements)
{
    QSet<QString> elementSet;
    for (const QString &element : elements) {
        elementSet.insert(element);
    }

    return elementSet;
}

static QStringList defaultFolderDockedElements()
{
    return resolveDockedElements({DOWNLOADS_PLACEHOLDER, APPLICATIONS_FOLDER_DOCK_ELEMENT});
}

static bool matchesLegacyDefaultDockedLayout(const QStringList &elements)
{
    const QSet<QString> expectedDesktopElements = asSet(stockDesktopDockedElements());
    const QSet<QString> expectedFolderElements = asSet(defaultFolderDockedElements());

    QSet<QString> desktopElements;
    QSet<QString> folderElements;
    for (const QString &element : elements) {
        if (element.startsWith(QStringLiteral("desktop/"))) {
            desktopElements.insert(element);
            continue;
        }

        if (element.startsWith(QStringLiteral("folder/"))) {
            folderElements.insert(element);
            continue;
        }

        return false;
    }

    if (desktopElements != expectedDesktopElements) {
        return false;
    }

    for (const QString &folderElement : folderElements) {
        if (!expectedFolderElements.contains(folderElement)) {
            return false;
        }
    }

    return true;
}

static QStringList mergedWithDefaultDockFolders(const QStringList &elements)
{
    QStringList mergedElements = elements;
    int insertIndex = mergedElements.indexOf(QStringLiteral("desktop/dde-file-manager"));
    if (insertIndex < 0) {
        insertIndex = -1;
    }

    for (const QString &folderElement : defaultFolderDockedElements()) {
        const int existingIndex = mergedElements.indexOf(folderElement);
        if (existingIndex >= 0) {
            insertIndex = existingIndex;
            continue;
        }

        mergedElements.insert(++insertIndex, folderElement);
    }

    return resolveDockedElements(mergedElements);
}
}

static inline QString bool2EnableStr(bool enable)
{
    return enable ? QStringLiteral("enabled") : QStringLiteral("disabled");
}

static inline bool enableStr2Bool(QString str)
{
    return str == QStringLiteral("enabled");
}

TaskManagerSettings* TaskManagerSettings::instance()
{
    static TaskManagerSettings* _taskManagerSettings = nullptr;
    if (!_taskManagerSettings) {
        _taskManagerSettings = new TaskManagerSettings();
    }
    return _taskManagerSettings;
}

TaskManagerSettings::TaskManagerSettings(QObject *parent)
    : QObject(parent)
    , m_taskManagerDconfig(DConfig::create(QStringLiteral("org.deepin.dde.shell"), QStringLiteral("org.deepin.ds.dock.taskmanager"), QString(), this))
{
    connect(m_taskManagerDconfig, &DConfig::valueChanged, this, [this](const QString &key){
        if (TASKMANAGER_ALLOWFOCEQUIT_KEY == key) {
            m_allowForceQuit = enableStr2Bool(m_taskManagerDconfig->value(TASKMANAGER_ALLOWFOCEQUIT_KEY).toString());
            Q_EMIT allowedForceQuitChanged();
        } else if (TASKMANAGER_SHOW_ATTENTION_ANIMATION_KEY == key) {
            m_showAttentionAnimation = m_taskManagerDconfig->value(TASKMANAGER_SHOW_ATTENTION_ANIMATION_KEY, true).toBool();
            Q_EMIT showAttentionAnimationChanged();
        } else if (TASKMANAGER_WINDOWSPLIT_KEY == key) {
            m_windowSplit = m_taskManagerDconfig->value(TASKMANAGER_WINDOWSPLIT_KEY).toBool();
            Q_EMIT windowSplitChanged();
        } else if (TASKMANAGER_DOCKEDELEMENTS_KEY == key) {
            m_dockedElements = resolveDockedElements(m_taskManagerDconfig->value(TASKMANAGER_DOCKEDELEMENTS_KEY, {}).toStringList());
            Q_EMIT dockedElementsChanged();
        }
    });

    m_allowForceQuit = enableStr2Bool(m_taskManagerDconfig->value(TASKMANAGER_ALLOWFOCEQUIT_KEY).toString());
    m_showAttentionAnimation = m_taskManagerDconfig->value(TASKMANAGER_SHOW_ATTENTION_ANIMATION_KEY, true).toBool();
    m_windowSplit = m_taskManagerDconfig->value(TASKMANAGER_WINDOWSPLIT_KEY).toBool();
    m_cgroupsBasedGrouping = m_taskManagerDconfig->value(TASKMANAGER_CGROUPS_BASED_GROUPING_KEY, true).toBool();
    m_dockedElements = resolveDockedElements(m_taskManagerDconfig->value(TASKMANAGER_DOCKEDELEMENTS_KEY, {}).toStringList());
    m_cgroupsBasedGroupingSkipAppIds = m_taskManagerDconfig->value(TASKMANAGER_CGROUPS_BASED_GROUPING_SKIP_APPIDS, {"deepin-terminal"}).toStringList();
    m_cgroupsBasedGroupingSkipCategories = m_taskManagerDconfig->value(TASKMANAGER_CGROUPS_BASED_GROUPING_SKIP_CATEGORIES, {"TerminalEmulator"}).toStringList();
    migrateFromDockedItems();
    migrateDefaultDockFolders();
}

bool TaskManagerSettings::isAllowedForceQuit()
{
    return m_allowForceQuit;
}

void TaskManagerSettings::setAllowedForceQuit(bool allowed)
{
    m_allowForceQuit = allowed;
    m_taskManagerDconfig->setValue(TASKMANAGER_ALLOWFOCEQUIT_KEY, bool2EnableStr(m_allowForceQuit));
}

bool TaskManagerSettings::showAttentionAnimation() const
{
    return m_showAttentionAnimation;
}

bool TaskManagerSettings::isWindowSplit()
{
    return m_windowSplit;
}

void TaskManagerSettings::setWindowSplit(bool split)
{
    m_windowSplit = split;
    m_taskManagerDconfig->setValue(TASKMANAGER_WINDOWSPLIT_KEY, m_windowSplit);
}

bool TaskManagerSettings::cgroupsBasedGrouping() const
{
    return m_cgroupsBasedGrouping;
}

QStringList TaskManagerSettings::cgroupsBasedGroupingSkipIds() const
{
    return m_cgroupsBasedGroupingSkipAppIds;
}

QStringList TaskManagerSettings::cgroupsBasedGroupingSkipCategories() const
{
    return m_cgroupsBasedGroupingSkipCategories;
}

QStringList TaskManagerSettings::dockedElements() const
{
    return m_dockedElements;
}

// elementId is like "desktop/sample.app.id"
bool TaskManagerSettings::isDocked(const QString &elementId) const
{
    return m_dockedElements.contains(elementId);
}

void TaskManagerSettings::migrateFromDockedItems()
{
    if (m_taskManagerDconfig->isDefaultValue(TASKMANAGER_DOCKEDITEMS_KEY)) {
        qDebug() << "Won't do migration since TASKMANAGER_DOCKEDITEMS_KEY is default value";
        return;
    } else if (!m_taskManagerDconfig->isDefaultValue(TASKMANAGER_DOCKEDELEMENTS_KEY)) {
        qDebug() << "Won't do migration since TASKMANAGER_DOCKEDELEMENTS_KEY is not default value";
        return;
    }

    QJsonArray legacyDockedItems;

    auto dcokedDesktopFilesStrList = m_taskManagerDconfig->value(TASKMANAGER_DOCKEDITEMS_KEY).toStringList();
    foreach(auto dcokedDesktopFilesStr, dcokedDesktopFilesStrList) {
        YAML::Node node;
        try {
            node = YAML::Load("{" + dcokedDesktopFilesStr.toStdString() + "}");
        } catch (const YAML::Exception&) {
            qWarning() << "unable to parse docked desktopfiles";
        }

        if (!node.IsMap()) continue;
        auto dockedItem = QJsonObject();
        for (auto it = node.begin(); it != node.end(); ++it) {
            auto key = it->first.as<std::string>();
            auto value = it->second.as<std::string>();
            dockedItem[QString::fromStdString(key)] = QString::fromStdString(value);
        }
        legacyDockedItems.append(dockedItem);
    }

    QStringList migratedDockedElements;
    for (auto dockedDesktopFile : std::as_const(legacyDockedItems)) {
        if (!dockedDesktopFile.isObject()) {
            continue;
        }
        auto dockedDesktopFileObj = dockedDesktopFile.toObject();
        if (dockedDesktopFileObj.contains(QStringLiteral("id")) && dockedDesktopFileObj.contains(QStringLiteral("type"))) {
            migratedDockedElements.append(QStringLiteral("desktop/%1").arg(dockedDesktopFileObj[QStringLiteral("id")].toString()));
        }
    }

    QStringList mergedDockedElements = m_dockedElements;
    for (const QString &element : std::as_const(migratedDockedElements)) {
        if (!mergedDockedElements.contains(element)) {
            mergedDockedElements.append(element);
        }
    }

    mergedDockedElements = resolveDockedElements(mergedDockedElements);
    if (mergedDockedElements != m_dockedElements) {
        m_dockedElements = mergedDockedElements;
        saveDockedElements();
    }
}

void TaskManagerSettings::migrateDefaultDockFolders()
{
    const int migrationVersion = m_taskManagerDconfig->value(TASKMANAGER_DEFAULT_DOCK_FOLDERS_MIGRATION_VERSION_KEY, 0).toInt();
    if (migrationVersion >= DEFAULT_DOCK_FOLDERS_MIGRATION_VERSION) {
        return;
    }

    QStringList migratedDockedElements = m_dockedElements;
    if (matchesLegacyDefaultDockedLayout(migratedDockedElements)) {
        migratedDockedElements = mergedWithDefaultDockFolders(migratedDockedElements);
    }

    const bool dockedElementsChanged = migratedDockedElements != m_dockedElements;
    if (dockedElementsChanged) {
        m_dockedElements = migratedDockedElements;
        saveDockedElements();
    }

    m_taskManagerDconfig->setValue(TASKMANAGER_DEFAULT_DOCK_FOLDERS_MIGRATION_VERSION_KEY,
                                   DEFAULT_DOCK_FOLDERS_MIGRATION_VERSION);
}

void TaskManagerSettings::saveDockedElements()
{
    m_taskManagerDconfig->setValue(TASKMANAGER_DOCKEDELEMENTS_KEY, m_dockedElements);
}

void TaskManagerSettings::setDockedElements(const QStringList &elements)
{
    m_dockedElements = elements;
    Q_EMIT dockedElementsChanged();
    saveDockedElements();
}

void TaskManagerSettings::toggleDockedElement(const QString &element)
{
    if (isDocked(element)) {
        removeDockedElement(element);
    } else {
        appendDockedElement(element);
    }
}

void TaskManagerSettings::appendDockedElement(const QString &element)
{
    if (m_dockedElements.contains(element)) {
        return;
    }

    m_dockedElements.append(element);
    Q_EMIT dockedElementsChanged();
    saveDockedElements();
}

void TaskManagerSettings::removeDockedElement(const QString &element)
{
    m_dockedElements.removeAll(element);
    Q_EMIT dockedElementsChanged();
    saveDockedElements();
}

}
