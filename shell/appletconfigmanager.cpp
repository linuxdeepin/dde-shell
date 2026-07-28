// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "appletconfigmanager.h"

#include "pluginloader.h"

#include <DConfig>
#include <QLoggingCategory>
#include <QSet>

DS_BEGIN_NAMESPACE

DCORE_USE_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(dsLoaderLog)

static constexpr auto AppId{"org.deepin.dde.shell"};
static constexpr auto ConfigName{"org.deepin.dde.shell"};
static constexpr auto EnableKey{"enable"};

AppletConfigManager::AppletConfigManager(QObject *parent)
    : QObject(parent)
{
}

QStringList AppletConfigManager::disabledPlugins() const
{
    return m_disabledPlugins;
}

bool AppletConfigManager::isAppletEnabled(const QString &pluginId) const
{
    return !m_disabledPlugins.contains(pluginId);
}

void AppletConfigManager::setAppletEnabled(const QString &pluginId, bool enabled)
{
    const bool currentlyEnabled = isAppletEnabled(pluginId);
    if (currentlyEnabled == enabled)
        return;

    if (enabled)
        m_disabledPlugins.removeAll(pluginId);
    else
        m_disabledPlugins.append(pluginId);
    Q_EMIT appletEnabledChanged(pluginId, enabled);
}

void AppletConfigManager::ensureAppletConfigs()
{
    auto loader = DPluginLoader::instance();
    const auto plugins = loader->plugins();
    QSet<QString> pluginIds;
    for (const auto &plugin : plugins) {
        pluginIds.insert(plugin.pluginId());
    }

    for (const auto &pluginId : pluginIds) {
        if (m_appletConfigs.contains(pluginId))
            continue;

        auto config = DConfig::create(QLatin1String(AppId),
                                      QLatin1String(ConfigName),
                                      QLatin1Char('/') + pluginId,
                                      this);
        m_appletConfigs.insert(pluginId, config);
        if (!config || !config->isValid()) {
            qCWarning(dsLoaderLog) << "Unable to create applet DConfig; applet remains enabled:" << pluginId;
            setAppletEnabled(pluginId, true);
            continue;
        }

        setAppletEnabled(pluginId, config->value(QLatin1String(EnableKey), true).toBool());
        QObject::connect(config, &DConfig::valueChanged, this, [this, pluginId, config](const QString &key) {
            if (key == QLatin1String(EnableKey))
                setAppletEnabled(pluginId, config->value(QLatin1String(EnableKey), true).toBool());
        });
    }
}

DS_END_NAMESPACE
