// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "widgetplugin.h"
#include "dockplugin.h"
#include "pluginsiteminterface.h"

#include <QPainter>
#include <QProcess>
#include <QVBoxLayout>

namespace dock {
WidgetPlugin::WidgetPlugin(PluginsItemInterface* pluginItem)
    : QObject()
    , m_pluginItem(pluginItem)
{
    QMetaObject::invokeMethod(this, [this](){
        m_pluginItem->init(this);
    });
}

WidgetPlugin::~WidgetPlugin()
{
}

void WidgetPlugin::itemAdded(PluginsItemInterface * const itemInter, const QString &itemKey)
{
    DockPlugin* plugin;

    if (m_pluginItem->flags() & Type_Common) {
        auto trayWidget = getQucikPluginTrayWidget();
        if (trayWidget) {
            trayWidget->setAttribute(Qt::WA_TranslucentBackground);
            plugin = getPlugin(trayWidget);
            plugin->setItemKey(itemKey);
            plugin->setPluginType(DockPlugin::Tray);
            plugin->setPluginFlags(m_pluginItem->flags());
            plugin->setPluginId(m_pluginItem->pluginName());
            plugin->setContextMenu(m_pluginItem->itemContextMenu(itemKey));

            connect(plugin, &DockPlugin::clicked, this, [this, plugin](const QString& menuId, bool checked){
                if (menuId.isEmpty()) {
                    QStringList commandArgument = itemCommand(plugin->itemKey()).split(" ");
                    if (commandArgument.size() > 0) {
                        QString command = commandArgument.first();
                        commandArgument.removeFirst();
                        QProcess::startDetached(command, commandArgument);
                    }
                } else {
                    m_pluginItem->invokedMenuItem(plugin->itemKey(), menuId, checked);
                }
            });

            trayWidget->show();
        }

        auto quickWidget = m_pluginItem->itemWidget(QUICK_ITEM_KEY);
        if (quickWidget) {
            quickWidget->setAttribute(Qt::WA_TranslucentBackground);
            plugin = getPlugin(quickWidget);
            plugin->setItemKey(itemKey);
            plugin->setPluginType(DockPlugin::Quick);
            plugin->setPluginFlags(m_pluginItem->flags());
            plugin->setPluginId(m_pluginItem->pluginName());
            plugin->setContextMenu(m_pluginItem->itemContextMenu(itemKey));
            quickWidget->show();
        }
    } else {
        auto widget = m_pluginItem->itemWidget(itemKey);
        if (widget) {
            widget->setAttribute(Qt::WA_TranslucentBackground);
            plugin = getPlugin(widget);
            plugin->setItemKey(itemKey);
            plugin->setPluginId(m_pluginItem->pluginName());
            plugin->setPluginFlags(m_pluginItem->flags());
            plugin->setContextMenu(m_pluginItem->itemContextMenu(itemKey));

            DockPlugin::PluginType type;
            if (m_pluginItem->flags() & Type_Fixed) {
                type = DockPlugin::Fixed;
            } else if (m_pluginItem->flags() & Type_System) {
                type = DockPlugin::System;
            } else if (m_pluginItem->flags() & Type_Tool) {
                type = DockPlugin::Tool;
            } else if (m_pluginItem->flags() & Type_Tray) {
                type = DockPlugin::Tray;
            }

            plugin->setPluginType(type);
            widget->show();
        }
    }

    auto popupWidget = m_pluginItem->itemPopupApplet(itemKey);
    if (!popupWidget && m_pluginItem->flags() & Type_Common) {
        popupWidget = m_pluginItem->itemPopupApplet(QUICK_ITEM_KEY);
    }

    if (popupWidget) {
        popupWidget->setAttribute(Qt::WA_TranslucentBackground);
        plugin = getPlugin(popupWidget);
        plugin->setPluginType(DockPlugin::Popup);
        plugin->setItemKey(itemKey);
        plugin->setPluginId(m_pluginItem->pluginName());
        popupWidget->show();
    }

    auto tipsWidget = m_pluginItem->itemTipsWidget(itemKey);
    if (tipsWidget) {
        tipsWidget->setAttribute(Qt::WA_TranslucentBackground);
        plugin = getPlugin(tipsWidget);
        plugin->setPluginType(DockPlugin::Tooltip);
        plugin->setItemKey(itemKey);
        plugin->setPluginId(m_pluginItem->pluginName());
        tipsWidget->show();
    }
}

void WidgetPlugin::itemUpdate(PluginsItemInterface * const itemInter, const QString &itemKey)
{
    if(m_widget) m_widget->update();

    auto widget = m_pluginItem->itemWidget(itemKey);
    if (widget) widget->update();

    auto quickPanel = m_pluginItem->itemWidget(QUICK_ITEM_KEY);
    if(quickPanel) quickPanel->update();

    auto popupWidget = m_pluginItem->itemPopupApplet(itemKey);
    if (popupWidget) popupWidget->update();

    auto tipsWidget = m_pluginItem->itemTipsWidget(itemKey);
    if (tipsWidget) tipsWidget->update();

}
void WidgetPlugin::itemRemoved(PluginsItemInterface * const itemInter, const QString &itemKey)
{
    auto widget = m_pluginItem->itemWidget(itemKey);
    if(widget) widget->hide();

    auto quickPanel = m_pluginItem->itemWidget(QUICK_ITEM_KEY);
    if(quickPanel) quickPanel->hide();

    auto popupWidget = m_pluginItem->itemPopupApplet(itemKey);
    if(popupWidget) popupWidget->hide();

    auto tipsWidget = m_pluginItem->itemTipsWidget(itemKey);
    if(tipsWidget) tipsWidget->hide();

}

void WidgetPlugin::requestWindowAutoHide(PluginsItemInterface * const itemInter, const QString &itemKey, const bool autoHide)
{
}

void WidgetPlugin::requestRefreshWindowVisible(PluginsItemInterface * const itemInter, const QString &itemKey)
{
}

void WidgetPlugin::requestSetAppletVisible(PluginsItemInterface * const itemInter, const QString &itemKey, const bool visible)
{
}

void WidgetPlugin::saveValue(PluginsItemInterface * const itemInter, const QString &key, const QVariant &value)
{
}

const QVariant WidgetPlugin::getValue(PluginsItemInterface *const itemInter, const QString &key, const QVariant& fallback)
{
    return fallback;
}

void WidgetPlugin::removeValue(PluginsItemInterface *const itemInter, const QStringList &keyList)
{
}

void WidgetPlugin::updateDockInfo(PluginsItemInterface *const, const DockPart &)
{
}


const QString WidgetPlugin::pluginName() const
{
    return m_pluginItem->pluginName();
}

const QString WidgetPlugin::itemCommand(const QString &itemKey)
{
    return m_pluginItem->itemCommand(itemKey);
}

const QString WidgetPlugin::itemContextMenu(const QString &itemKey)
{
    return m_pluginItem->itemContextMenu(itemKey);
}

void WidgetPlugin::handleClicked(const QString &itemKey, const QString &menuId, const bool checked)
{
    if (menuId.isEmpty()) {
        QStringList commandArgument = itemCommand(itemKey).split(" ");
        if (commandArgument.size() > 0) {
            QString command = commandArgument.first();
            commandArgument.removeFirst();
            QProcess::startDetached(command, commandArgument);
        }
    } else {
        m_pluginItem->invokedMenuItem(itemKey, menuId, checked);
    }
}

void WidgetPlugin::onDockPositionChanged(uint32_t position)
{
    qApp->setProperty(PROP_POSITION, position);
    m_pluginItem->positionChanged(static_cast<Dock::Position>(position));
}

void WidgetPlugin::onDockDisplayModeChanged(uint32_t displayMode)
{
    qApp->setProperty(PROP_DISPLAY_MODE, displayMode);
    m_pluginItem->displayModeChanged(static_cast<Dock::DisplayMode>(displayMode));
}

QWidget* WidgetPlugin::getQucikPluginTrayWidget()
{
    auto trayIcon = m_pluginItem->icon(DockPart::QuickShow);
    if (trayIcon.isNull())
        return m_widget.get();

    if (m_widget.isNull()) {
        auto func = [this]() -> QPixmap {
            auto trayIcon = m_pluginItem->icon(DockPart::QuickShow);
            QSize size = trayIcon.availableSizes().size() > 0 ? trayIcon.availableSizes().last() : QSize(18, 18) * qApp->devicePixelRatio();
            auto pixmap = trayIcon.pixmap(24, 24);
            pixmap.setDevicePixelRatio(qApp->devicePixelRatio());
            return pixmap;
        };

        m_widget.reset(new TrayIconWidget(func));

        connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, [this](){
            auto widget = getQucikPluginTrayWidget();
            if (widget) widget->update();
        }, Qt::UniqueConnection);
    }

    return m_widget.get();
}

DockPlugin* WidgetPlugin::getPlugin(QWidget* widget)
{
    widget->setParent(nullptr);
    widget->winId();
    return DockPlugin::get(widget->windowHandle());
}

TrayIconWidget::TrayIconWidget(std::function<QPixmap()> trayIconCallback, QWidget* parent)
    : QWidget(parent)
    , m_callBack(trayIconCallback)
{
    setFixedSize(48, 48);
}

TrayIconWidget::~TrayIconWidget()
{}

void TrayIconWidget::paintEvent(QPaintEvent *event)
{
    auto pixmap = m_callBack();
    QPainter painter(this);
    QSize size = QCoreApplication::testAttribute(Qt::AA_UseHighDpiPixmaps) ? pixmap.size() / qApp->devicePixelRatio() : pixmap.size();
    QRect pixmapRect = QRect(QPoint((rect().width() - size.width()) / 2, (rect().height() - size.height()) / 2), size);
    painter.drawPixmap(pixmapRect, pixmap);
}
}
