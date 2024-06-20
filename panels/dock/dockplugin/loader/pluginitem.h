// SPDX-FileCopyrightText: 2011 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef PLUGINSITEM_H
#define PLUGINSITEM_H

#include "pluginsiteminterface_v2.h"

#include <QWidget>

class QMenu;
class PluginItem : public QWidget
{
    Q_OBJECT

public:
    explicit PluginItem(PluginsItemInterface *pluginsItemInterface, const QString &itemKey, QWidget *parent = nullptr);
    ~PluginItem() override;

    void init();
    void updateItemWidgetSize(const QSize &size);

    int pluginFlags() const;
    void setPluginFlags(int flags);

protected:
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;

    virtual QWidget *centralWidget();
    virtual void mouseRightButtonClicked();
    virtual void showPluginTooltip();

    PluginsItemInterface * pluginsItemInterface();
    void initPluginMenu();
    void showTooltip(const QString &itemKey);

private:
    void mouseLeftButtonClicked();

private:
    void updatePopupSize(const QRect &rect);

protected:
    QString m_itemKey;
    QMenu *m_menu;

private:
    PluginsItemInterface *m_pluginsItemInterface;
    PluginsItemInterfaceV2 *m_pluginsItemInterfacev2;

    int m_pluginFlags = 0;
    bool m_isPanelPopupShow = false;
};

#endif // PLUGINSITEM_H

