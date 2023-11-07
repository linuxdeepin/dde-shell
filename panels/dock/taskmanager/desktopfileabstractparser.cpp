// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dsglobal.h"
#include "appitem.h"
#include "desktopfileabstractparser.h"

#include <QPointer>

DS_BEGIN_NAMESPACE

namespace dock {

void DesktopfileAbstractParser::addAppItem(QPointer<AppItem> item)
{
    if (m_appitems.contains(item)) return;

    connect(item.get(), &QObject::destroyed, this, [this](){
        auto item = qobject_cast<AppItem*>(sender());
        m_appitems.removeAll(item);
    });

    m_appitems.append(item);
}

QPointer<AppItem> DesktopfileAbstractParser::getAppItem()
{
    return m_appitems.size() > 0 ? m_appitems.first() : nullptr;
}
}
DS_END_NAMESPACE
