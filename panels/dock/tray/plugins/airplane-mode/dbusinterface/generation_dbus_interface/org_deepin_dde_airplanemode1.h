/*
 * This file was generated by qdbusxml2cpp-fix version 0.8
 * Command line was: qdbusxml2cpp-fix -c org_deepin_dde_AirplaneMode1 -p /home/uos/private/github_projects/dde-shell/panels/dock/tray/plugins/airplane-mode/dbusinterface/generation_dbus_interface/org_deepin_dde_airplanemode1 /home/uos/private/github_projects/dde-shell/panels/dock/tray/plugins/airplane-mode/dbusinterface/xml/org.deepin.dde.AirplaneMode1.xml
 *
 * qdbusxml2cpp-fix is Copyright (C) 2016 Deepin Technology Co., Ltd.
 *
 * This is an auto-generated file.
 * Do not edit! All changes made to it will be lost.
 */

#ifndef ORG_DEEPIN_DDE_AIRPLANEMODE1_H
#define ORG_DEEPIN_DDE_AIRPLANEMODE1_H

#include <QtCore/QObject>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>

#include <DDBusExtendedAbstractInterface>
#include <QtDBus/QtDBus>


/*
 * Proxy class for interface org.deepin.dde.AirplaneMode1
 */
class __org_deepin_dde_AirplaneMode1Private;
class __org_deepin_dde_AirplaneMode1 : public DTK_CORE_NAMESPACE::DDBusExtendedAbstractInterface
{
    Q_OBJECT

public:
    static inline const char *staticInterfaceName()
    { return "org.deepin.dde.AirplaneMode1"; }

public:
    explicit __org_deepin_dde_AirplaneMode1(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent = 0);

    ~__org_deepin_dde_AirplaneMode1();

    Q_PROPERTY(bool BluetoothEnabled READ bluetoothEnabled NOTIFY BluetoothEnabledChanged)
    bool bluetoothEnabled();

    Q_PROPERTY(bool Enabled READ enabled NOTIFY EnabledChanged)
    bool enabled();

    Q_PROPERTY(bool WifiEnabled READ wifiEnabled NOTIFY WifiEnabledChanged)
    bool wifiEnabled();

public Q_SLOTS: // METHODS
    inline QDBusPendingReply<> DumpState()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QStringLiteral("DumpState"), argumentList);
    }

    inline void DumpStateQueued()
    {
        QList<QVariant> argumentList;

        CallQueued(QStringLiteral("DumpState"), argumentList);
    }


    inline QDBusPendingReply<> Enable(bool enabled)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(enabled);
        return asyncCallWithArgumentList(QStringLiteral("Enable"), argumentList);
    }

    inline void EnableQueued(bool enabled)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(enabled);

        CallQueued(QStringLiteral("Enable"), argumentList);
    }


    inline QDBusPendingReply<> EnableBluetooth(bool enabled)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(enabled);
        return asyncCallWithArgumentList(QStringLiteral("EnableBluetooth"), argumentList);
    }

    inline void EnableBluetoothQueued(bool enabled)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(enabled);

        CallQueued(QStringLiteral("EnableBluetooth"), argumentList);
    }


    inline QDBusPendingReply<> EnableWifi(bool enabled)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(enabled);
        return asyncCallWithArgumentList(QStringLiteral("EnableWifi"), argumentList);
    }

    inline void EnableWifiQueued(bool enabled)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(enabled);

        CallQueued(QStringLiteral("EnableWifi"), argumentList);
    }



Q_SIGNALS: // SIGNALS
    // begin property changed signals
    void BluetoothEnabledChanged(bool  value) const;
    void EnabledChanged(bool  value) const;
    void WifiEnabledChanged(bool  value) const;

public Q_SLOTS:
    void CallQueued(const QString &callName, const QList<QVariant> &args);

private Q_SLOTS:
    void onPendingCallFinished(QDBusPendingCallWatcher *w);
    void onPropertyChanged(const QString &propName, const QVariant &value);

private:
    __org_deepin_dde_AirplaneMode1Private *d_ptr;
};

namespace org {
  namespace deepin {
    namespace dde {
      typedef ::__org_deepin_dde_AirplaneMode1 AirplaneMode1;
    }
  }
}
#endif
