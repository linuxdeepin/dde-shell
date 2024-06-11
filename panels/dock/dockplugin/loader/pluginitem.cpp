// SPDX-FileCopyrightText: 2011 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "constants.h"
#include "pluginitem.h"
#include "plugin.h"

#include <QHBoxLayout>
#include <QMouseEvent>
#include <qglobal.h>
#include <qobject.h>

const static Dock::PluginFlags UNADAPTED_PLUGIN_FLAGS = Dock::PluginFlag::Type_Unadapted | Dock::PluginFlag::Attribute_Normal;

PluginItem::PluginItem(PluginsItemInterface *pluginItemInterface, const QString &itemKey, QWidget *parent)
    : QWidget(parent)
    , m_pluginInterface(pluginItemInterface)
    , m_pluginInterfacev2(dynamic_cast<PluginsItemInterfaceV2*>(pluginItemInterface))
    , m_centralWidget(pluginItemInterface->itemWidget(itemKey))
    , m_itemKey(itemKey)
    , m_menu(new QMenu(this))
{
    setAttribute(Qt::WA_TranslucentBackground);

    auto hLayout = new QHBoxLayout;
    hLayout->addWidget(m_centralWidget);
    hLayout->setContentsMargins(0, 0, 0, 0);
    hLayout->setMargin(0);

    setLayout(hLayout);
}

PluginItem::~PluginItem() = default;

int PluginItem::flags()
{
    if (m_pluginInterfacev2) {
        return m_pluginInterfacev2->flags();
    }
    auto obj = dynamic_cast<QObject*>(m_pluginInterface);
    if (!obj) {
        qWarning() << "failed to convert v1 to object!";
    }
    bool ok;
    auto flags = obj->property("pluginFlags").toInt(&ok);
    if (ok) {
        return flags;
    }

    return UNADAPTED_PLUGIN_FLAGS;
}

void PluginItem::mouseLeftButtonClicked()
{
    const QString command = m_pluginInterface->itemCommand(m_itemKey);
    if (!command.isEmpty()) {
        qInfo() << "command: " << command;
        auto *proc = new QProcess(this);

        connect(proc, static_cast<void (QProcess::*)(int)>(&QProcess::finished), proc, &QProcess::deleteLater);

        proc->startDetached(command);
        return;
    }

    if (auto popup = m_pluginInterface->itemPopupApplet(m_itemKey)) {
        if (popup->isVisible()) {
            popup->hide();
            return;
        }

        popup->setAttribute(Qt::WA_TranslucentBackground);
        popup->winId();

        auto geometry = windowHandle()->geometry();
        auto pluginPopup = Plugin::PluginPopup::get(popup->windowHandle());
        pluginPopup->setPopupType(Plugin::PluginPopup::PopupTypePanel);
        pluginPopup->setX(geometry.x() + geometry.width() / 2), pluginPopup->setY(geometry.y() + geometry.height() / 2);
        popup->show();
    }
}

void PluginItem::mouseRightButtonClicked()
{
    if (m_menu->actions().isEmpty()) {
        const QString menuJson = m_pluginInterface->itemContextMenu(m_itemKey);
        if (menuJson.isEmpty())
            return;

        QJsonDocument jsonDocument = QJsonDocument::fromJson(menuJson.toLocal8Bit().data());
        if (jsonDocument.isNull())
            return;

        QJsonObject jsonMenu = jsonDocument.object();

        QJsonArray jsonMenuItems = jsonMenu.value("items").toArray();
        for (auto item : jsonMenuItems) {
            QJsonObject itemObj = item.toObject();
            QAction *action = new QAction(itemObj.value("itemText").toString());
            action->setCheckable(itemObj.value("isCheckable").toBool());
            action->setChecked(itemObj.value("checked").toBool());
            action->setData(itemObj.value("itemId").toString());
            action->setEnabled(itemObj.value("isActive").toBool());
            m_menu->addAction(action);
        }
    }

    m_menu->setAttribute(Qt::WA_TranslucentBackground, true);
    // FIXME: qt5integration drawMenuItemBackground will draw a background event is's transparent
    auto pa = this->palette();
    pa.setColor(QPalette::ColorRole::Window, Qt::transparent);
    m_menu->setPalette(pa);
    m_menu->winId();

    auto geometry = windowHandle()->geometry();
    auto pluginPopup = Plugin::PluginPopup::get(m_menu->windowHandle());
    pluginPopup->setPopupType(Plugin::PluginPopup::PopupTypeMenu);
    pluginPopup->setX(geometry.x() + geometry.width() / 2), pluginPopup->setY(geometry.y() + geometry.height() / 2);
    m_menu->setFixedSize(m_menu->sizeHint());
    m_menu->exec();
}

void PluginItem::mousePressEvent(QMouseEvent *e)
{
    QWidget::mousePressEvent(e);
}

void PluginItem::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        mouseLeftButtonClicked();
    } else if (e->button() == Qt::RightButton) {
        mouseRightButtonClicked();
    }
    QWidget::mouseReleaseEvent(e);
}

void PluginItem::enterEvent(QEvent *event)
{
    auto popup = m_pluginInterface->itemPopupApplet(m_itemKey);
    if (popup)
        popup->hide();

    QMetaObject::invokeMethod(this, [this](){
        auto toolTip = m_pluginInterface->itemTipsWidget(m_itemKey);
        if (!toolTip) {
            toolTip = m_pluginInterface->itemTipsWidget(Dock::QUICK_ITEM_KEY);
        }

        if (!toolTip) {
            qDebug() << "no tooltip";
            return;
        }

        toolTip->setParent(nullptr);
        toolTip->setAttribute(Qt::WA_TranslucentBackground);
        toolTip->winId();

        auto geometry = windowHandle()->geometry();
        auto pluginPopup = Plugin::PluginPopup::get(toolTip->windowHandle());
        pluginPopup->setPopupType(Plugin::PluginPopup::PopupTypeTooltip);
        pluginPopup->setX(geometry.x() + geometry.width() / 2), pluginPopup->setY(geometry.y() + geometry.height() / 2);
        if (toolTip->sizeHint().width() > 0 && toolTip->sizeHint().height() > 0) {
            toolTip->setFixedSize(toolTip->sizeHint());
        }
        toolTip->show();
    });
}

void PluginItem::leaveEvent(QEvent *event)
{
    auto tooltip = m_pluginInterface->itemTipsWidget(m_itemKey);
    if (tooltip && tooltip->windowHandle())
        tooltip->windowHandle()->hide();
}
