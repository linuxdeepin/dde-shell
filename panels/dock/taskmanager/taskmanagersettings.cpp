// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "globals.h"
#include "dsglobal.h"
#include "taskmanagersettings.h"

#include <QJsonObject>
#include <QJsonDocument>

#include <algorithm>
#include <iterator>

#include <string>
#include <unordered_map>
#include <yaml-cpp/exceptions.h>
#include <yaml-cpp/node/node.h>
#include <yaml-cpp/node/parse.h>
#include <yaml-cpp/yaml.h>

DS_BEGIN_NAMESPACE
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
    , m_taskManagerDconfig(DConfig::create("org.deepin.ds.dock", "org.deepin.ds.dock.taskmanager", QString(), this))
{
    connect(m_taskManagerDconfig, &DConfig::valueChanged, this, [this](const QString &key){
        if (TASKMANAGER_ALLOWFOCEQUIT_KEY == key) {
            m_allowForceQuit = enableStr2Bool(m_taskManagerDconfig->value(TASKMANAGER_ALLOWFOCEQUIT_KEY).toString());
            Q_EMIT allowedForceQuitChanged();
        } else if (TASKMANAGER_WINDOWSPLIT_KEY == key) {
            m_windowSplit = enableStr2Bool(m_taskManagerDconfig->value(TASKMANAGER_WINDOWSPLIT_KEY).toString());
            Q_EMIT windowSplitChanged();
        } else if (TASKMANAGER_DOCKEDAPPIDS_KEY == key) {
            loadDockedDesktopFiles();
            Q_EMIT dockedDesktopFilesChanged();
        }
    });

    m_allowForceQuit = enableStr2Bool(m_taskManagerDconfig->value(TASKMANAGER_ALLOWFOCEQUIT_KEY).toString());
    m_windowSplit = enableStr2Bool(m_taskManagerDconfig->value(TASKMANAGER_WINDOWSPLIT_KEY).toString());
    loadDockedDesktopFiles();
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
    m_taskManagerDconfig->setValue(TASKMANAGER_WINDOWSPLIT_KEY, bool2EnableStr(m_windowSplit));
}

void TaskManagerSettings::dockedDesktopFilesPersisted()
{
    QStringList list;

    for (auto dockedDesktopFile : m_dockedApps) {
        if (!dockedDesktopFile.isObject()) continue;
        YAML::Node node;
        auto dockedDesktopFileObj = dockedDesktopFile.toObject();
        for (auto key : dockedDesktopFileObj.keys()) {
            node[key.toStdString()] = dockedDesktopFileObj[key].toString().toStdString();
        }
        auto str = QString::fromStdString(YAML::Dump(node));
        list << str.replace("\n",",");
    }

    m_taskManagerDconfig->setValue(TASKMANAGER_DOCKEDAPPIDS_KEY, list);
}

void TaskManagerSettings::loadDockedDesktopFiles()
{
    while (!m_dockedApps.isEmpty()) m_dockedApps.removeLast();

    auto dcokedDesktopFilesStrList = m_taskManagerDconfig->value(TASKMANAGER_DOCKEDAPPIDS_KEY).toStringList();
    foreach(auto dcokedDesktopFilesStr, dcokedDesktopFilesStrList) {
        YAML::Node node;
        try {
            node = YAML::Load("{" + dcokedDesktopFilesStr.toStdString() + "}");
        } catch (YAML::Exception) {
            qWarning() << "unable to parse docked desktopfiles";
        }

        if (!node.IsMap()) continue;
        auto dockedDesktopFile = QJsonObject();
        for (auto it = node.begin(); it != node.end(); ++it) {
            auto key = it->first.as<std::string>();
            auto value = it->second.as<std::string>();
            dockedDesktopFile[QString::fromStdString(key)] = QString::fromStdString(value);
        }
        m_dockedApps.append(dockedDesktopFile);
    }
}

void TaskManagerSettings::setDockedDesktopFiles(QJsonArray desktopfiles)
{
    m_dockedApps = desktopfiles;
    dockedDesktopFilesPersisted();
}

void TaskManagerSettings::appnedDockedDesktopfiles(QJsonObject desktopfile)
{
    m_dockedApps.append(desktopfile);
    dockedDesktopFilesPersisted();
}

void TaskManagerSettings::removeDockedDesktopfile(QJsonObject desktopfile)
{
    for (int i = 0; i < m_dockedApps.count(); i++) {
        if (m_dockedApps.at(i) == desktopfile) {
            m_dockedApps.removeAt(i);
            return;
        }
    }
}

QJsonArray TaskManagerSettings::dockedDesktopFiles()
{
    return m_dockedApps;
}

}

DS_END_NAMESPACE
