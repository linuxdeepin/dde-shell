// SPDX-FileCopyrightText: 2018 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef NETITEM_H
#define NETITEM_H

#include <DStackedWidget>

#include <QAbstractListModel>
#include <QModelIndex>
#include <QJsonObject>
#include <QStyledItemDelegate>

#include <networkdevicebase.h>

class NetworkDevice;
class NetItem;
class WirelessConnect;
class QLabel;
class QPushButton;
class NetworkPanel;

namespace dde {
    namespace network {
        class NetworkDeviceBase;
        class WiredDevice;
        class WirelessDevice;
        class AccessPoints;
        class WiredConnection;
        class WirelessConnection;
        enum class DeviceType;
    } // namespace network
} // namespace dde

using namespace dde::network;
namespace Dtk {
    namespace Widget {
        class DListView;
        class DViewItemAction;
        class DLoadingIndicator;
        class DSpinner;
        class DPasswordEdit;
        class DLineEdit;
        class DStandardItem;
    } // namespace Widget
} // namespace Dtk

DWIDGET_USE_NAMESPACE

enum NetItemRole {
    TypeRole = Qt::UserRole + 100,
    DeviceDataRole,
    DataRole,
    ConnectionStatusRole,
    MouseInBoundingRole,
    DeviceTypeRole,
    sortRole
};

#define PANELWIDTH 330

enum NetItemType {
    DeviceControllViewItem = 0, // 总控开关
    WirelessControllViewItem,   // 无线网卡开关
    WirelessViewItem,           // 无线列表
    WirelessHiddenViewItem,     // 无线隐藏列表
    WiredControllViewItem,      // 有线网卡开关
    WiredViewItem               // 有线列表
};

enum NetConnectionType {
    UnConnected = 0, // 未连接
    Connecting,      // 正在连接
    Connected        // 已连接
};

class NetItem : public QObject
{
    Q_OBJECT

public:
    NetItem(QWidget *parent);
    virtual ~NetItem();

    virtual DStandardItem *standardItem();
    virtual void updateView() {}
    virtual NetItemType itemType() = 0;

private:
    DStandardItem *m_standardItem;
    QWidget *m_parentWidget;
};

class DeviceControllItem : public NetItem
{
    Q_OBJECT

public:
    DeviceControllItem(const DeviceType &deviceType, QWidget *parent);
    ~DeviceControllItem() Q_DECL_OVERRIDE;

    void setDevices(const QList<NetworkDeviceBase *> &devices);
    DeviceType deviceType();
    void updateView() Q_DECL_OVERRIDE;
    NetItemType itemType() Q_DECL_OVERRIDE;

private:
    void initItemText();

private:
    QList<NetworkDeviceBase *> m_devices;
    DeviceType m_deviceType;
};

class WiredControllItem : public NetItem
{
    Q_OBJECT

public:
    WiredControllItem(QWidget *parent, WiredDevice *device);
    ~WiredControllItem() Q_DECL_OVERRIDE;

    WiredDevice *device();
    void updateView() Q_DECL_OVERRIDE;
    NetItemType itemType() Q_DECL_OVERRIDE;

private:
    WiredDevice *m_device;
};

class WirelessControllItem : public NetItem
{
    Q_OBJECT

public:
    WirelessControllItem(QWidget *parent, WirelessDevice *device);
    ~WirelessControllItem() Q_DECL_OVERRIDE;

    WirelessDevice *device();
    void updateView() Q_DECL_OVERRIDE;
    NetItemType itemType() Q_DECL_OVERRIDE;

private:
    WirelessDevice *m_device;
};

class WiredItem : public NetItem
{
    Q_OBJECT

public:
    WiredItem(QWidget *parent, WiredDevice *device, WiredConnection *connection);
    ~WiredItem() Q_DECL_OVERRIDE;

    WiredConnection *connection();
    void updateView() Q_DECL_OVERRIDE;
    NetItemType itemType() Q_DECL_OVERRIDE;
    void connectNetwork();

private:
    void initUi();
    QString symbolicIcon(const bool &connected) const;
    void initConnection();

private:
    WiredConnection *m_connection;
    WiredDevice *m_device;
    DViewItemAction *m_connectionIconAction;
};

class WirelessItem : public NetItem
{
    Q_OBJECT

Q_SIGNALS:
    void sizeChanged();

public:
    enum ExpandWidget {
        Hide = -1,
        ShowPassword,
        ShowSSID,
    };

public:
    WirelessItem(QWidget *parent, WirelessDevice *device, AccessPoints *ap, NetworkPanel *panel);
    ~WirelessItem() Q_DECL_OVERRIDE;

    const AccessPoints *accessPoint();
    const WirelessDevice *wirelessDevice();
    void updateView() Q_DECL_OVERRIDE;
    NetItemType itemType() Q_DECL_OVERRIDE;
    static QString getStrengthStateString(int strength);
    void expandWidget(ExpandWidget type, bool autoDisconnect = true);
    void connectNetwork();
    void expandPasswordInput();
    bool expandVisible();

public Q_SLOTS:
    void onAirplaneModeChanged(bool airplaneModeEnabled);
    void onCancel();

private:
    void initUi(QWidget *parent);
    void initConnection();
    void updateSrcirityIcon();
    void updateWifiIcon();
    void updateConnectionStatus();
    void createPasswordEdit();
    void createSsidEdit();
    void initExpandUi();
    const QDateTime timeStamp(WirelessConnection *connection) const;

private Q_SLOTS:
    void onConnection();
    void onConnectNetwork();
    void onInputPassword(const QString oldPassword);
    void onConnectHidden();
    void checkInputValid();
    void onApConnectionStatusChanged(ConnectionStatus);

private:
    AccessPoints *m_accessPoint;
    WirelessDevice *m_device;
    DViewItemAction *m_securityAction;
    DViewItemAction *m_wifiLabel;
    DViewItemAction *m_connectionAction;
    DSpinner *m_loadingStat;

    QWidget *m_connectionWidget;

    DStackedWidget *m_stackWidget;
    DViewItemAction *m_expandItem;
    DViewItemAction *m_topItem; // 占空间

    DPasswordEdit *m_passwdEdit;
    DLineEdit *m_ssidEdit;

    WirelessConnect *m_wirelessConnect;
    DPushButton *m_connectButton;
    NetworkPanel *m_panel;
};

Q_DECLARE_METATYPE(NetItemType)
Q_DECLARE_METATYPE(QList<NetworkDeviceBase *>)
Q_DECLARE_METATYPE(DeviceType)

#endif //  NETWORKAPPLETMODEL_H
