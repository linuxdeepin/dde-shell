// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QList>

#include "abstractdesktopinfo.h"
#include "applicationinterface.h"

namespace apps {
class DesktopInfoAM : public AbstractDesktopInfo
{
    Q_OBJECT

public:
    explicit DesktopInfoAM(const QString &amDbusPath, QObject* parent = nullptr);
    explicit DesktopInfoAM(const QString &amDbusPath, const ObjectInterfaceMap &source, QObject* parent = nullptr);
    ~DesktopInfoAM();

    void launch(const QString& action = {}, const QStringList &fields = {}, const QVariantMap &options = {});

    // desktop static info
    QString desktopId() const;
    QString name();
    QString iconName();
    QString genericName();
    bool nodisplay();
    QStringList categories();
    QList<QPair<QString, QString>> actions();
    qint64 lastLaunchedTime();
    qint64 installedTime();
    qint64 launchedTimes();
    QString deepinVendor();
    QString startupWMClass();
    bool autoStart() const;
    void setAutoStart(bool autoStart);

    bool onDesktop() const;
    void setOnDesktop(bool onDesktop);

private:
    QString getLocaleOrDefaultValue(const QStringMap &value, const QString &targetKey, const QString &fallbackKey);

// update desktop static info
private Q_SLOTS:
    void onPropertyChanged(const QDBusMessage &msg);

private:
    QString m_id;
    QString m_path;
    QString m_name;
    QString m_iconName;
    QString m_genericName;
    QStringList m_categies;
    QString m_startUpWMClass;
    QString m_xDeepinVendor;
    QList<QPair<QString, QString>> m_actions;
    qint64 m_lastLaunchedTime;
    qint64 m_installedTime;
    qint64 m_launchedTimes;
    bool m_nodisplay;
    bool m_autoStart;
    bool m_isDocked;
    bool m_isOnDesktop;

    using Application = org::desktopspec::ApplicationManager1::Application;
    Application *m_desktopInfo;
};
}
