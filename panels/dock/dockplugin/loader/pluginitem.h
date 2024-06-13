// SPDX-FileCopyrightText: 2011 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef PLUGINSITEM_H
#define PLUGINSITEM_H

#include "pluginsiteminterface_v2.h"

#include <QWidget>
#include <QMenu>

class PluginItem : public QWidget
{
    Q_OBJECT

public:
    explicit PluginItem(PluginsItemInterface *pluginInterface, const QString &itemKey, const QString &quickItemKey = QString(), QWidget *parent = nullptr);
    ~PluginItem() override;
    static int flags(QPluginLoader *pluginLoader, PluginsItemInterface *pluginsItemInterface);

    void updateItemWidgetSize(const QSize &size);

protected:
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    /*void mouseMoveEvent(QMouseEvent *e) override;*/
    /*void contextMenuEvent(QContextMenuEvent *e) override;*/
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    /*bool eventFilter(QObject *watched, QEvent *event) override;*/
    /*void showEvent(QShowEvent *event) override;*/
private:
    void mouseLeftButtonClicked();
    void mouseRightButtonClicked();

private:
    void updatePopupSize(const QRect &rect);

private:
    PluginsItemInterface *m_pluginInterface;
    PluginsItemInterfaceV2 *m_pluginInterfacev2;
    QWidget *m_centralWidget;
    QString m_itemKey;
    QString m_quickItemKey;
    QMenu *m_menu;

    bool m_isPanelPopupShow = false;
    static QPluginLoader *m_pluginLoader;
};

#endif // PLUGINSITEM_H

