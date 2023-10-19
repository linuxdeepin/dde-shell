// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "containmentitem.h"

#include <QLoggingCategory>
#include <QQmlEngine>

DS_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(dsLog)

DContainmentItem::DContainmentItem(QQuickItem *parent)
    : DAppletItem(parent)
{
}

DContainmentItem::~DContainmentItem()
{
}

DContainment *DContainmentItem::qmlAttachedProperties(QObject *object)
{
    return qobject_cast<DContainment *>(DAppletItem::qmlAttachedProperties(object));
}

DS_END_NAMESPACE
