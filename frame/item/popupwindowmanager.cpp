// SPDX-FileCopyrightText: 2019 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "popupwindowmanager.h"
#include "dockitem.h"

#include <QDBusConnection>
#include <QApplication>

PopupWindowManager::PopupWindowManager(QWidget* parent)
    : QObject(parent)
    , m_appletPopup(createPopupWindow(false, false))
    , m_tipsPopup(createPopupWindow(true, true))
{
    QDBusConnection::sessionBus().connect("com.deepin.dde.lockFront", "/com/deepin/dde/lockFront", "com.deepin.dde.lockFront", "Visible",
        this, SLOT(hidePopup(bool)));
    m_tipsPopup->setObjectName("PopupWindowManager");
    m_appletPopup->setEnableSystemMove(false);
}

PopupWindowManager::~PopupWindowManager()
{
    delete m_appletPopup;
    m_appletPopup = nullptr;
    delete m_tipsPopup;
    m_tipsPopup = nullptr;
}

DockPopupWindow* PopupWindowManager::appletPopup() const
{
    return m_appletPopup;
}

DockPopupWindow* PopupWindowManager::toAppletPopup(DockItem* item)
{
    return toPopup(m_appletPopup, item);
}

DockPopupWindow* PopupWindowManager::toTipsPopup(DockItem* item)
{
    return toPopup(m_tipsPopup, item);
}

DockPopupWindow* PopupWindowManager::popup(DockItem* item) const
{
    return m_bindMapping.value(item, nullptr);
}

bool PopupWindowManager::isModel(DockItem* item) const
{
    auto itemPopup = m_bindMapping.value(item, nullptr);
    return itemPopup ? itemPopup->isModel() : true;
}

bool PopupWindowManager::existVisibleModelPopup() const
{
    return (m_appletPopup->isModel() && m_appletPopup->isVisible())
        || (m_tipsPopup->isModel() && m_tipsPopup->isVisible());
}

void PopupWindowManager::hide(DockItem* item) const
{
    auto itemPopup = m_bindMapping.value(item, nullptr);
    if (itemPopup)
        itemPopup->hide();
}

void PopupWindowManager::accept(DockItem* item) const
{
    auto itemPopup = m_bindMapping.value(item, nullptr);
    if (itemPopup)
        emit itemPopup->accept();
}

bool PopupWindowManager::isVisible(DockItem* item) const
{
    auto itemPopup = m_bindMapping.value(item, nullptr);
    if (itemPopup)
        return itemPopup->isVisible();

    return false;
}

QWidget* PopupWindowManager::getContent(DockItem* item) const
{
    auto itemPopup = m_bindMapping.value(item, nullptr);
    if (itemPopup)
        return itemPopup->getContent();

    return nullptr;
}

void PopupWindowManager::show(DockItem* item, const QPoint& pos, const bool model)
{
    auto itemPopup = m_bindMapping.value(item, nullptr);
    if (itemPopup)
        itemPopup->show(pos, model);
}

DockPopupWindow* PopupWindowManager::createPopupWindow(bool noFocus, bool tellWmDock) const
{
    DockPopupWindow* arrowRectangle = new DockPopupWindow(nullptr, noFocus, tellWmDock);
    connect(qApp, &QApplication::aboutToQuit, arrowRectangle, &DockPopupWindow::deleteLater);
    return arrowRectangle;
}

DockPopupWindow* PopupWindowManager::toPopup(DockPopupWindow* popup, DockItem* item)
{
    for (const auto &item : m_bindMapping.keys()) {
        if (m_bindMapping.value(item) == popup) {
            m_bindMapping.remove(item);
        }
    }

    m_bindMapping.insert(item, popup);
    return popup;
}

void PopupWindowManager::hidePopup(bool visible)
{
    if (m_appletPopup && m_appletPopup->isVisible() && visible)
        m_appletPopup->hide();
}

void PopupWindowManager::disconnectAccept(DockItem *item) const
{
    auto itemPopup = m_bindMapping.value(item, nullptr);
    if (itemPopup && item) {
        disconnect(itemPopup, &DockPopupWindow::accept, item, &DockItem::popupWindowAccept);
    }
}

void PopupWindowManager::connectAccept(DockItem *item) const
{
    auto itemPopup = m_bindMapping.value(item, nullptr);
    if (itemPopup && item) {
        connect(itemPopup, &DockPopupWindow::accept, item, &DockItem::popupWindowAccept, Qt::UniqueConnection);
    }
}
