// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
#include <QObject>
#include <QVariantMap>

namespace apps {
class AbstractDesktopInfo : public QObject
{
    Q_OBJECT

    // READ Only from desktopfile
    Q_PROPERTY(QString desktopId READ desktopId FINAL CONSTANT)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(QString iconName READ iconName NOTIFY iconNameChanged)
    Q_PROPERTY(bool nodisplay READ nodisplay NOTIFY nodisplayChanged)
    Q_PROPERTY(QStringList categories READ categories NOTIFY categoriesChanged)
    Q_PROPERTY(QList<QPair<QString, QString>> actions READ actions NOTIFY actionsChanged)
    Q_PROPERTY(qint64 lastLaunchedTime READ lastLaunchedTime NOTIFY lastLaunchedTimeChanged)
    Q_PROPERTY(qint64 installedTime READ installedTime NOTIFY installedTimeChanged)
    Q_PROPERTY(QString deepinVendor READ deepinVendor NOTIFY deepinVendorChanged)
    Q_PROPERTY(QString startupWMClass READ startupWMClass NOTIFY startupWMClassChanged)

    // READ and WRITE Status
    Q_PROPERTY(bool autoStart READ autoStart WRITE setAutoStart NOTIFY autoStartChanged)
    Q_PROPERTY(bool onDesktop READ onDesktop WRITE setOnDesktop NOTIFY onDesktopChanged)

public:
    explicit AbstractDesktopInfo(QObject* parent = nullptr) {};

    virtual void launch(const QString& action = {}, const QStringList &fields = {}, const QVariantMap &options = {}) = 0;

    // desktop static info
    virtual QString desktopId() const = 0;
    virtual QString name() = 0;
    virtual QString iconName() = 0;
    virtual QString genericName() = 0;
    virtual bool nodisplay() = 0;
    virtual QStringList categories() = 0;
    virtual QList<QPair<QString, QString>> actions() = 0;
    virtual qint64 lastLaunchedTime() = 0;
    virtual qint64 installedTime() = 0;
    virtual qint64 launchedTimes() = 0;
    virtual QString deepinVendor() = 0;
    virtual QString startupWMClass() = 0;
    virtual bool autoStart() const = 0;
    virtual void setAutoStart(bool autoStart) = 0;

    virtual bool onDesktop() const = 0;
    virtual void setOnDesktop(bool onDesktop) = 0;

Q_SIGNALS:
    void nameChanged();
    void iconNameChanged();
    void nodisplayChanged();
    void categoriesChanged();
    void actionsChanged();
    void lastLaunchedTimeChanged();
    void installedTimeChanged();
    void launchedTimesChanged();
    void autoStartChanged();
    void onDesktopChanged();
    void deepinVendorChanged();
    void startupWMClassChanged();
};
}
