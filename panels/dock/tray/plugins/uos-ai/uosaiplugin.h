// SPDX-FileCopyrightText: 2011 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UOSAIPLUGIN_H
#define UOSAIPLUGIN_H

#include "uosaiwidget.h"
#include "pluginsiteminterface.h"

#include <QApplication>
#include <QDBusInterface>
#include <QLabel>
#include <QList>
#include <QMap>
#include <QVariant>
#include <QJsonDocument>

#define DOCK_DEFAULT_POS    9
#define KEY_DNDMODE         0
#define KEY_SHOWICON        5

#define PLUGIN_INTERFACE  PluginsItemInterface

class QGSettings;
namespace uos_ai {

class UosAiPlugin : public QObject, PLUGIN_INTERFACE
{
    Q_OBJECT
    Q_INTERFACES(PluginsItemInterface)
    Q_PLUGIN_METADATA(IID "com.deepin.dock.PluginsItemInterface" FILE "uosai.json")

public:
    explicit UosAiPlugin(QObject *parent = nullptr);

    const QString pluginName() const override;
    const QString pluginDisplayName() const override;
    void init(PluginProxyInterface *proxyInter) override;
    QWidget *itemWidget(const QString &itemKey) override;
    QWidget *itemTipsWidget(const QString &itemKey) override;
    const QString itemCommand(const QString &itemKey) override;
    int itemSortKey(const QString &itemKey) override;
    void setSortKey(const QString &itemKey, const int order) override;

    void pluginStateSwitched() override;
    bool pluginIsAllowDisable() override { return true; }
    bool pluginIsDisable() override;
#ifdef USE_V23_DOCK
    QIcon icon(const DockPart &dockPart, DGuiApplicationHelper::ColorType themeType) override;
#endif
#ifdef USE_DOCK_API_V2
    Dock::PluginFlags flags() const override { return Dock::Type_System | Dock::Attribute_Normal; }
    void setMessageCallback(MessageCallbackFunc cb) override { m_messageCallback = cb; }
#endif

private:
    QPixmap loadSvg(QString &iconName, const QSize size, const qreal ratio = qApp->devicePixelRatio());
    void loadPlugin();

private slots:
    void changeTheme();
#ifdef USE_DOCK_API_V2
    void onUosAiVisibleChanged(bool);
#endif

private:
    bool m_pluginLoaded = false;
    UosAiWidget *m_itemWidget = nullptr;
    QLabel *m_tipsLabel;
    QScopedPointer<QWidget> m_quickWidget;
#ifdef USE_DOCK_API_V2
    MessageCallbackFunc m_messageCallback;
#endif
};

}

#endif // UOSAIPLUGIN_H
