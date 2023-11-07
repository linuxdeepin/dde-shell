// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dsglobal.h"

#include <QObject>

DS_BEGIN_NAMESPACE
namespace dock {
class AbstractItem : public QObject
{
    Q_OBJECT
    // indetifier
    Q_PROPERTY(QString id READ id)
    Q_PROPERTY(ItemType itemType READ itemType)

    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(QString menus READ menus NOTIFY menusChanged)
    Q_PROPERTY(QString icon READ icon NOTIFY iconChanged FINAL)
    Q_PROPERTY(bool isActive READ isActive NOTIFY activeChanged)

    Q_PROPERTY(bool isDocked READ isDocked WRITE setDocked NOTIFY dockedChanged)

public:
    enum ItemType {
        AppType,
        GroupType,
        FloderType,
        /* if treat belows as appitem
        ShowDesktopType,
        WorkSpacePreviewType,
        */
    };

    Q_ENUM(ItemType)

    ~AbstractItem();

    virtual QString id() = 0;
    virtual ItemType itemType() = 0;

    virtual QString icon() = 0;
    virtual QString name() = 0;
    virtual QString menus() = 0;

    virtual bool isActive() = 0;
    virtual void active() = 0;

    virtual bool isDocked() = 0;
    virtual void setDocked(bool docked) =0;

    virtual void handleClick(const QString& clickItem) = 0;

protected:
    AbstractItem(QString id, QObject *parent = nullptr);

Q_SIGNALS:
    void nameChanged();
    void iconChanged();
    void menusChanged();

    void activeChanged();
    void dockedChanged();

};
}
DS_END_NAMESPACE
