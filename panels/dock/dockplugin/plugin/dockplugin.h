// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QWindow>

namespace dock {
class DockPluginPrivate;

class Q_DECL_EXPORT DockPlugin : public QObject
{
    Q_OBJECT
    Q_PROPERTY(PluginType pluginType READ pluginType WRITE setPluginType NOTIFY pluginTypeChanged)
    Q_PROPERTY(QString dccIcon READ dccIcon WRITE setDCCIcon NOTIFY dccIconChanged)
    Q_PROPERTY(QString pluginId READ pluginId WRITE setPluginId NOTIFY pluginIdChanged)
    Q_PROPERTY(QString itemKey READ itemKey WRITE setItemKey NOTIFY itemKeyChanged)
    Q_PROPERTY(QString contextMenu READ contextMenu WRITE setContextMenu NOTIFY contextMenuChanged)
    Q_PROPERTY(int pluginFlags READ pluginFlags WRITE setPluginFlags NOTIFY pluginFlagsChnaged)

public:
    enum PluginType {
        Tooltip = 1,
        Popup,
        Tray,
        Fixed,
        System,
        Tool,
        Quick,
        SlidingPanel
    };
    Q_ENUM(PluginType)

    ~DockPlugin();

    PluginType pluginType() const;
    void setPluginType(PluginType type);

    QString dccIcon() const;
    void setDCCIcon(const QString& icon);

    QString pluginId() const;
    void setPluginId(const QString& pluginid);

    QString itemKey() const;
    void setItemKey(const QString& itemKey);

    QString contextMenu() const;
    void setContextMenu(const QString& contextMenu);

    int pluginFlags() const;
    void setPluginFlags(int flags);

    static DockPlugin* get(QWindow* window);

    static DockPlugin *qmlAttachedProperties(QObject *object);

Q_SIGNALS:
    void clicked(const QString&, bool);
    void dockPositionChanged(uint32_t position);
    void dockColorThemeChanged(uint32_t colorType);
    void dockDisplayModeChanged(uint32_t displayMode);

Q_SIGNALS:
    void pluginTypeChanged();
    void trayIconChnaged();
    void dccIconChanged();
    void pluginIdChanged();
    void itemKeyChanged();
    void contextMenuChanged();
    void pluginFlagsChnaged();

private:
    explicit DockPlugin(QWindow* window);
    QScopedPointer<DockPluginPrivate> d;
};
}
