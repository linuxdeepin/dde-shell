// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "globals.h"
#include "dsglobal.h"
#include "desktopfileamparsersettings.h"

DS_BEGIN_NAMESPACE
namespace dock {
DesktopFileAMParserSettings::DesktopFileAMParserSettings(QObject *parent)
    : QObject(parent)
    , m_dconfig(DConfig::create("org.deepin.ds.dock", "org.deepin.ds.dock.taskmanager.am", QString(), this))
{
    connect(m_dconfig, &DConfig::valueChanged, this, [this](const QString& key){
        if (key == AM_DOCKEDAPPIDS_KEY) {
            Q_EMIT dockedAMDesktopfileIdsChanged();
        }
    });
}

DesktopFileAMParserSettings* DesktopFileAMParserSettings::instance()
{
    DesktopFileAMParserSettings* _settings = nullptr;
    if (!_settings) {
        _settings = new DesktopFileAMParserSettings();
    }

    return _settings;
}

void DesktopFileAMParserSettings::setDockedAMDektopfileIds(QStringList ids)
{
    m_dconfig->setValue(AM_DOCKEDAPPIDS_KEY, ids);
}

QStringList DesktopFileAMParserSettings::dockedAMDektopfileIds()
{
    return m_dconfig->value(AM_DOCKEDAPPIDS_KEY).toStringList();
}

}

DS_END_NAMESPACE
