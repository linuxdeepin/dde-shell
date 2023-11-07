// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dsglobal.h"

#include <QList>
#include <QObject>
#include <QPointer>

DS_BEGIN_NAMESPACE

namespace dock {
class AppItem;

class DesktopfileAbstractParser : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString id READ id)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(QString desktopIcon READ desktopIcon NOTIFY desktopIcon)
    Q_PROPERTY(bool isDocked READ isDocked WRITE setDocked NOTIFY dockedChanged)
    Q_PROPERTY(QString genericName READ genericName NOTIFY genericNameChanged)
    Q_PROPERTY(QList<QPair<QString, QString>> actions READ actions NOTIFY actionsChanged)

public:
    DesktopfileAbstractParser(QString desktopid, QObject* parent = nullptr) : QObject(parent) {};

    virtual QString id() = 0;
    virtual QString name() = 0;
    virtual QList<QPair<QString, QString>> actions() = 0;
    virtual QString genericName() = 0;
    virtual QString desktopIcon() = 0;
    virtual bool isDocked() = 0;
    virtual bool isValied() = 0;

    virtual void launch() = 0;
    virtual void launchWithAction(const QString& action) = 0;
    virtual void requestQuit() = 0;
    virtual void setDocked(bool docked) = 0;

    void addAppItem(QPointer<AppItem> item);
    QPointer<AppItem> getAppItem();

Q_SIGNALS:
    void nameChanged();
    void iconChanged();
    void actionsChanged();
    void genericNameChanged();
    void dockedChanged();

protected:
    QList<QPointer<AppItem>> m_appitems;
};
}
DS_END_NAMESPACE
