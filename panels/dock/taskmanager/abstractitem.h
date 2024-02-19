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
    Q_PROPERTY(QString id READ id FINAL CONSTANT)
    Q_PROPERTY(ItemType itemType READ itemType FINAL CONSTANT)

    Q_PROPERTY(QString name READ name NOTIFY nameChanged FINAL)
    Q_PROPERTY(QString menus READ menus NOTIFY menusChanged FINAL)
    Q_PROPERTY(QString icon READ icon NOTIFY iconChanged FINAL)

    Q_PROPERTY(bool isActive READ isActive NOTIFY activeChanged FINAL)
    Q_PROPERTY(bool isDocked READ isDocked WRITE setDocked NOTIFY dockedChanged FINAL)

    Q_PROPERTY(QStringList windows READ windows NOTIFY windowsChanged FINAL)
    Q_PROPERTY(QStringList desktopFileIDs READ desktopFileIDs NOTIFY desktopFileIDsChanged FINAL)
    Q_PROPERTY(QString dockedDir READ dockedDir NOTIFY dockedDirChanged FINAL)

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

    virtual QString id() const = 0;
    virtual QString type() const = 0;
    virtual ItemType itemType() const = 0;

    virtual QString icon() const = 0;
    virtual QString name() const = 0;
    virtual QString menus() const = 0;

    virtual bool isActive() const = 0;
    virtual void active() const = 0;

    virtual bool isDocked() const = 0;
    virtual void setDocked(bool docked) =0;

    virtual void handleClick(const QString& clickItem) = 0;

    // three type data

    // app item
    virtual QStringList windows() {return QStringList();};

    // appgroup item
    virtual QStringList desktopFileIDs() {return QStringList();};

    // floder item
    virtual QString dockedDir() {return QString();};

protected:
    AbstractItem(const QString& id, QObject *parent = nullptr);

Q_SIGNALS:
    void nameChanged();
    void iconChanged();
    void menusChanged();

    void activeChanged();
    void dockedChanged();

    void windowsChanged();
    void desktopFileIDsChanged();
    void dockedDirChanged();

};
}
DS_END_NAMESPACE
