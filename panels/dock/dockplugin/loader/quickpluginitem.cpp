// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "quickpluginitem.h"
#include "constants.h"
#include "plugin.h"

#include <QMouseEvent>
#include <QMenu>

QuickPluginItem::QuickPluginItem(PluginsItemInterface *pluginInterface, const QString &itemKey, QWidget *parent)
    : PluginItem(pluginInterface, itemKey, parent)
    , m_onDockAction(nullptr)
{
}

QWidget *QuickPluginItem::centralWidget()
{
    if (pluginsItemInterface()) {
        auto quickItem = pluginsItemInterface()->itemWidget(Dock::QUICK_ITEM_KEY);
        quickItem->setVisible(true);
        return quickItem;
    }
    return nullptr;
}

void QuickPluginItem::mouseReleaseEvent(QMouseEvent *e)
{
     if (e->button() == Qt::RightButton) {
        if (auto menu = pluginContextMenu()) {
            if (auto pluginPopup = Plugin::PluginPopup::get(menu->windowHandle())) {
                auto geometry = windowHandle()->geometry();
                const auto offset = e->pos();
                pluginPopup->setX(geometry.x() + offset.x());
                pluginPopup->setY(geometry.y() + offset.y());
                menu->exec();
            }
        }
        e->accept();
    }

    QWidget::mouseReleaseEvent(e);
}

QMenu *QuickPluginItem::pluginContextMenu()
{
    if (m_menu->isEmpty()) {
        initPluginMenu();

        int flags = pluginFlags();
        if (flags & Dock::Attribute_CanSetting) {
            m_onDockAction = new QAction();
            m_onDockAction->setEnabled(true);
            m_menu->addAction(m_onDockAction);
        }
    }

    if (m_onDockAction) {
        bool onDock = true; // TODO 要找到对应插件是否在任务栏上显示，1070通过dbus获取
        QString itemText = onDock ? tr("Remove from dock") : tr("Pin to dock");
        m_onDockAction->setText(itemText);
        m_onDockAction->setData(onDock ? Dock::unDockMenuItemId : Dock::dockMenuItemId);
    }

    m_menu->setAttribute(Qt::WA_TranslucentBackground, true);
    // FIXME: qt5integration drawMenuItemBackground will draw a background event is's transparent
    auto pa = m_menu->palette();
    pa.setColor(QPalette::ColorRole::Window, Qt::transparent);
    m_menu->setPalette(pa);
    m_menu->winId();

    auto geometry = windowHandle()->geometry();
    auto pluginPopup = Plugin::PluginPopup::get(m_menu->windowHandle());
    pluginPopup->setPluginId(pluginsItemInterface()->pluginName());
    pluginPopup->setItemKey(Dock::QUICK_ITEM_KEY);
    pluginPopup->setPopupType(Plugin::PluginPopup::PopupTypeMenu);
    m_menu->setFixedSize(m_menu->sizeHint());
    return m_menu;
}

QWidget *QuickPluginItem::pluginTooltip()
{
    auto popup = pluginsItemInterface()->itemPopupApplet(m_itemKey);
    if (popup && popup->isVisible())
        popup->windowHandle()->hide();

    return itemTooltip(Dock::QUICK_ITEM_KEY);
}

void QuickPluginItem::mousePressEvent(QMouseEvent *e)
{
    Q_EMIT recvMouseEvent(e->type());

    PluginItem::mousePressEvent(e);
}
