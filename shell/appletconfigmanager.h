// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dsglobal.h"

#include <QObject>
#include <QMap>
#include <QStringList>

namespace Dtk::Core {
class DConfig;
}

DS_BEGIN_NAMESPACE

class AppletConfigManager : public QObject
{
    Q_OBJECT
public:
    explicit AppletConfigManager(QObject *parent = nullptr);

    QStringList disabledPlugins() const;
    bool isAppletEnabled(const QString &pluginId) const;
    void ensureAppletConfigs();

Q_SIGNALS:
    void appletEnabledChanged(const QString &pluginId, bool enabled);

private:
    void setAppletEnabled(const QString &pluginId, bool enabled);

    QMap<QString, Dtk::Core::DConfig *> m_appletConfigs;
    QStringList m_disabledPlugins;
};

DS_END_NAMESPACE
