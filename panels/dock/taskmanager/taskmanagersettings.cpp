// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "globals.h"
#include "taskmanagersettings.h"

#include <QJsonObject>
#include <QJsonDocument>

#include <string>

#include <yaml-cpp/yaml.h>

namespace dock {
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
        } else if (TASKMANAGER_WINDOWSPLIT_KEY == key) {
            m_windowSplit = m_taskManagerDconfig->value(TASKMANAGER_WINDOWSPLIT_KEY).toBool();
            Q_EMIT windowSplitChanged();
        } else if (TASKMANAGER_DOCKEDELEMENTS_KEY == key) {
            m_dockedElements = m_taskManagerDconfig->value(TASKMANAGER_DOCKEDELEMENTS_KEY, {}).toStringList();
            Q_EMIT dockedElementsChanged();
        }
    });

    m_allowForceQuit = enableStr2Bool(m_taskManagerDconfig->value(TASKMANAGER_ALLOWFOCEQUIT_KEY).toString());
    m_windowSplit = m_taskManagerDconfig->value(TASKMANAGER_WINDOWSPLIT_KEY).toBool();
    m_cgroupsBasedGrouping = m_taskManagerDconfig->value(TASKMANAGER_CGROUPS_BASED_GROUPING_KEY, true).toBool();
    m_dockedElements = m_taskManagerDconfig->value(TASKMANAGER_DOCKEDELEMENTS_KEY, {}).toStringList();
    m_cgroupsBasedGroupingSkipAppIds = m_taskManagerDconfig->value(TASKMANAGER_CGROUPS_BASED_GROUPING_SKIP_APPIDS, {"deepin-terminal"}).toStringList();
    m_cgroupsBasedGroupingSkipCategories = m_taskManagerDconfig->value(TASKMANAGER_CGROUPS_BASED_GROUPING_SKIP_CATEGORIES, {"TerminalEmulator"}).toStringList();
    migrateFromDockedItems();
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

    for (auto dockedDesktopFile : std::as_const(legacyDockedItems)) {
        if (!dockedDesktopFile.isObject()) {
            continue;
        }
        auto dockedDesktopFileObj = dockedDesktopFile.toObject();
        if (dockedDesktopFileObj.contains(QStringLiteral("id")) && dockedDesktopFileObj.contains(QStringLiteral("type"))) {
            m_dockedElements.append(QStringLiteral("desktop/%1").arg(dockedDesktopFileObj[QStringLiteral("id")].toString()));
        }
    }
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
