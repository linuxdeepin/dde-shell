// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dsglobal.h"
#include "dockpanel.h"
#include "constants.h"
#include "dockiteminfo.h"

#include <QObject>
#include <QDBusContext>
#include <QDBusArgument>

/** this class used for old dock api compatible
  * it will forward old dbus call to new implementation
  */
namespace dock {
class DockDBusProxy final: public QObject, public QDBusContext
{
    Q_OBJECT
    Q_PROPERTY(QRect geometry READ geometry NOTIFY geometryChanged FINAL)
    Q_PROPERTY(QRect FrontendWindowRect READ frontendWindowRect NOTIFY FrontendWindowRectChanged FINAL)
    Q_PROPERTY(Position Position READ position WRITE setPosition NOTIFY PositionChanged FINAL)

    Q_PROPERTY(HideMode HideMode READ hideMode WRITE setHideMode NOTIFY hideModeChanged FINAL)
    Q_PROPERTY(HideState HideState READ hideState NOTIFY hideStateChanged FINAL)

public:
    DockDBusProxy(DockPanel* parent = nullptr);

    void setItemOnDock(const QString &settingKey, const QString &itemKey, bool visible);
    void setPluginVisible(const QString &pluginName, bool visible);
    bool getPluginVisible(const QString &pluginName);
    QString getPluginKey(const QString &pluginName);
    void resizeDock(int offset, bool dragging);
    QStringList GetLoadedPlugins();
    DockItemInfos plugins();
    void ReloadPlugins();
    void callShow();

    QRect geometry();
    QRect frontendWindowRect();

    Position position();
    void setPosition(Position position);

    HideMode hideMode();
    void setHideMode(HideMode mode);

    HideState hideState();
    bool RequestDock(const QString &desktopFile, int index);
    bool IsDocked(const QString &desktopFile);
    bool RequestUndock(const QString &desktopFile);

Q_SIGNALS:
    void geometryChanged();
    void hideModeChanged(HideMode mode);
    void hideStateChanged(HideState state);

    void PositionChanged(Position position);
    void FrontendWindowRectChanged(QRect rect);

private:
    DockPanel* parent() const;
    QString getAppID(const QString &desktopfile);
    QList<DS_NAMESPACE::DApplet *> appletList(const QString &pluginId) const;
    DS_NAMESPACE::DApplet *applet(const QString &pluginId) const;

    DS_NAMESPACE::DApplet *m_oldDockApplet;
    DS_NAMESPACE::DApplet *m_clipboardApplet;
    DS_NAMESPACE::DApplet *m_searchApplet;
};
}

