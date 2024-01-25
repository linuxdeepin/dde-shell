// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dsglobal.h"
#include "appitem.h"
#include "desktopfileabstractparser.h"
#include "taskmanagersettings.h"

#include <QPointer>
#include <QJsonObject>

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

bool DesktopfileAbstractParser::isDocked()
{
    if (!isValied().first) {
        qDebug() << isValied().second;
        return false;
    }

    QJsonObject desktopfile;
    desktopfile["type"] = appType();
    desktopfile["id"] = id();
    return TaskManagerSettings::instance()->dockedDesktopFiles().contains(desktopfile);
}

void DesktopfileAbstractParser::setDocked(bool docked)
{
    qDebug() << isValied() << docked;
    if (!isValied().first) {
        qDebug() << isValied().second;
        return;
    }

    QJsonObject desktopfile;
    desktopfile["type"] = appType();
    desktopfile["id"] = id();

    if (docked) {
        TaskManagerSettings::instance()->appnedDockedDesktopfiles(desktopfile);
    } else {
        TaskManagerSettings::instance()->removeDockedDesktopfile(desktopfile);
    }
}

}
DS_END_NAMESPACE
