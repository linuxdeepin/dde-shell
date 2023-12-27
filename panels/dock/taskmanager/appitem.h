// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dsglobal.h"
#include "abstractwindow.h"

#include <QMap>
#include <QObject>
#include <QPointer>

DS_BEGIN_NAMESPACE
namespace dock {
class TaskManager;
class DesktopfileAbstractParser;

class AppItem : public QObject
{
    Q_OBJECT
    // indetifier
    Q_PROPERTY(QString id READ id)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(QString menus READ menus NOTIFY menusChanged)
    Q_PROPERTY(QString icon READ icon NOTIFY iconChanged FINAL)
    Q_PROPERTY(bool isActive READ isActive NOTIFY activeChanged)
    Q_PROPERTY(bool isDocked READ isDocked WRITE setDocked NOTIFY dockedChanged)
    Q_PROPERTY(QStringList windows READ windows NOTIFY itemWindowCountChanged)

public:
    ~AppItem();

    QString id() const;
    QString icon() const;
    QString name() const;
    QString menus() const;

    QString desktopfileID() const;

    bool isActive() const;
    void active();

    bool isDocked() const;
    void setDocked(bool docked);

    QStringList windows() const;

    bool hasWindow() const;

    void appendWindow(QPointer<AbstractWindow> window);
    void removeWindow(QPointer<AbstractWindow> window);

    void setDesktopFileParser(QSharedPointer<DesktopfileAbstractParser> desktopfile);
    QPointer<DesktopfileAbstractParser> getDesktopFileParser();

    void launch();
    void requestQuit();
    void handleMenu(const QString& menuId);

protected:
    friend class TaskManager;
    AppItem(QString id, QObject *parent = nullptr);

Q_SIGNALS:
    void nameChanged();
    void iconChanged();
    void menusChanged();
    void genericNameChanged();

    void activeChanged();
    void dockedChanged();
    void itemWindowCountChanged();
    void currentActiveWindowChanged();

private:
    void updateCurrentActiveWindow(QPointer<AbstractWindow> window);
    void checkAppItemNeedDeleteAndDelete();

private Q_SLOTS:
    void onWindowDestroyed();

private:
    QString m_id;
    QList<QPointer<AbstractWindow>> m_windows;
    QPointer<AbstractWindow> m_currentActiveWindow;
    QSharedPointer<DesktopfileAbstractParser> m_desktopfileParser;

};
}
DS_END_NAMESPACE
