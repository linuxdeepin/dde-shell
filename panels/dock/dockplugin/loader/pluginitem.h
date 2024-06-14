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
    explicit PluginItem(PluginsItemInterface *pluginsItemInterface, const QString &itemKey, QWidget *parent = nullptr);
    ~PluginItem() override;

    static int flags(QPluginLoader *pluginLoader, PluginsItemInterface *pluginsItemInterface);

    void init();
    void updateItemWidgetSize(const QSize &size);

protected:
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    virtual QWidget *centralWidget();
    PluginsItemInterface * pluginsItemInterface();

private:
    void mouseLeftButtonClicked();
    void mouseRightButtonClicked();

private:
    void updatePopupSize(const QRect &rect);

private:
    PluginsItemInterface *m_pluginsItemInterface;
    PluginsItemInterfaceV2 *m_pluginsItemInterfacev2;
    QString m_itemKey;
    QMenu *m_menu;

    bool m_isPanelPopupShow = false;
    static QPluginLoader *m_pluginLoader;
};

#endif // PLUGINSITEM_H

