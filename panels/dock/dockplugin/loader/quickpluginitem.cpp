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
        return pluginsItemInterface()->itemWidget(Dock::QUICK_ITEM_KEY);
    }
    return nullptr;
}

void QuickPluginItem::mouseReleaseEvent(QMouseEvent *e)
{
     if (e->button() == Qt::RightButton) {
        mouseRightButtonClicked();
        e->accept();
    }

    QWidget::mouseReleaseEvent(e);
}

void QuickPluginItem::mouseRightButtonClicked()
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
    pluginPopup->setX(geometry.x() + geometry.width() / 2);
    pluginPopup->setY(geometry.y() + geometry.height() / 2);
    m_menu->setFixedSize(m_menu->sizeHint());
    m_menu->exec();
}

void QuickPluginItem::showPluginTooltip()
{
    showTooltip(Dock::QUICK_ITEM_KEY);
}

