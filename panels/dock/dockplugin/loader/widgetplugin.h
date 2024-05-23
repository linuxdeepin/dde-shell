// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "pluginproxyinterface.h"

#include <QMenu>
#include <QLabel>
#include <QObject>
#include <QWindow>
#include <QScopedPointer>

namespace Plugin {
class EmbemdPlugin;
}
namespace dock {
class TrayIconWidget;
class WidgetPlugin : public QObject, public PluginProxyInterface
{
    Q_OBJECT
    Q_PROPERTY(QString pluginId READ pluginName)

public:
    WidgetPlugin(PluginsItemInterface* pluginItem);
    ~WidgetPlugin();

    // proxy interface
    void itemAdded(PluginsItemInterface * const itemInter, const QString &itemKey) override;
    void itemUpdate(PluginsItemInterface * const itemInter, const QString &itemKey) override;
    void itemRemoved(PluginsItemInterface * const itemInter, const QString &itemKey) override;
    void requestWindowAutoHide(PluginsItemInterface * const itemInter, const QString &itemKey, const bool autoHide) override;
    void requestRefreshWindowVisible(PluginsItemInterface * const itemInter, const QString &itemKey) override;
    void requestSetAppletVisible(PluginsItemInterface * const itemInter, const QString &itemKey, const bool visible) override;
    void saveValue(PluginsItemInterface * const itemInter, const QString &key, const QVariant &value) override;
    const QVariant getValue(PluginsItemInterface *const itemInter, const QString &key, const QVariant& fallback = QVariant()) override;
    void removeValue(PluginsItemInterface *const itemInter, const QStringList &keyList) override;
    void updateDockInfo(PluginsItemInterface *const, const DockPart &) override;

    const QString pluginName() const;
    const QString itemCommand(const QString &itemKey);
    const QString itemContextMenu(const QString &itemKey);

public Q_SLOTS:
    void onDockPositionChanged(uint32_t position);
    void onDockDisplayModeChanged(uint32_t displayMode);

private:
    QWidget* getQucikPluginTrayWidget(const QString &itemKey);
    Plugin::EmbemdPlugin* getPlugin(QWidget*);

private:
    PluginsItemInterface* m_pluginItem;

    QScopedPointer<TrayIconWidget> m_widget;
};

class TrayIconWidget : public QWidget
{
    Q_OBJECT
public:
    TrayIconWidget(PluginsItemInterface* m_pluginItem, QString m_itemKey, QWidget* parent = nullptr);
    ~TrayIconWidget();

    void paintEvent(QPaintEvent *event) override;

protected:
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    PluginsItemInterface* m_pluginItem;
    QString m_itemKey;
    QMenu *m_menu;
};
}
