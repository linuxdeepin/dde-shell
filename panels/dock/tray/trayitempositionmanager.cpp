// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "trayitempositionmanager.h"

#include <QTimer>
#include <QDebug>

namespace docktray {

// style of UI.
static const int itemSize = 16;
static const int itemPadding = 4;
static const int itemSpacing = 2;
static const QSize itemVisualSize = QSize(itemSize + itemPadding * 2, itemSize + itemPadding * 2);

Qt::Orientation TrayItemPositionManager::orientation() const
{
    return m_orientation;
}

int TrayItemPositionManager::dockHeight() const
{
    return m_dockHeight;
}

TrayItemPositionManager::TrayItemPositionManager(QObject *parent)
    : QObject(parent)
{
    m_itemSpacing = itemSpacing;
    m_itemPadding = itemPadding;
    m_itemVisualSize = itemVisualSize;
}

}
