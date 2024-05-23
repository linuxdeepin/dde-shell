// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QWindow>
#include <QString>
#include <qt6/QtGui/qwindow.h>

namespace Plugin {
class EmbemdPluginPrivate;

class Q_DECL_EXPORT EmbemdPlugin : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString itemKey READ itemKey WRITE setItemKey NOTIFY itemKeyChanged)
    Q_PROPERTY(QString pluginId READ pluginId WRITE setPluginId NOTIFY pluginIdChanged)
    Q_PROPERTY(int pluginType READ pluginType WRITE setPluginType NOTIFY pluginTypeChanged)
    Q_PROPERTY(int pluginFlags READ pluginFlags WRITE setPluginFlags NOTIFY pluginFlagsChanged)
    Q_PROPERTY(uint32_t pluginOrder READ pluginOrder WRITE setPluginOrder NOTIFY pluginOrderChanged)

public:

    ~EmbemdPlugin();

    QString pluginId() const;
    void setPluginId(const QString& pluginid);

    QString itemKey() const;
    void setItemKey(const QString& itemKey);

    int pluginFlags() const;
    void setPluginFlags(int flags);

    int pluginType() const;
    void setPluginType(int type);

    uint32_t pluginOrder() const;
    void setPluginOrder(uint32_t order);

    static EmbemdPlugin* get(QWindow* window);
    static bool contains(QWindow* window);

Q_SIGNALS:
    void message(const QString &msg);

Q_SIGNALS:
    void itemKeyChanged();
    void pluginIdChanged();
    void pluginTypeChanged();
    void pluginFlagsChanged();
    void pluginOrderChanged();

private:
    explicit EmbemdPlugin(QWindow* window);
    QScopedPointer<EmbemdPluginPrivate> d;
};


class PluginPopupPrivate;

class Q_DECL_EXPORT PluginPopup : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString itemKey READ itemKey WRITE setItemKey NOTIFY itemKeyChanged)
    Q_PROPERTY(QString pluginId READ pluginId WRITE setPluginId NOTIFY pluginIdChanged)
    Q_PROPERTY(int popupType READ popupType WRITE setPopupType NOTIFY popupTypeChanged)

    Q_PROPERTY(int x READ x WRITE setX NOTIFY xChanged)
    Q_PROPERTY(int y READ y WRITE setY NOTIFY yChanged)

public:
    ~PluginPopup();

    QString pluginId() const;
    void setPluginId(const QString& pluginid);

    QString itemKey() const;
    void setItemKey(const QString& itemKey);

    int popupType() const;
    void setPopupType(const int& type);

    int x() const;
    void setX(const int& x);

    int y() const;
    void setY(const int& y);

    static PluginPopup* get(QWindow* window);
    static bool contains(QWindow* window);

Q_SIGNALS:
    void itemKeyChanged();
    void pluginIdChanged();
    void popupTypeChanged();

    void xChanged();
    void yChanged();

private:
    explicit PluginPopup(QWindow* window);
    QScopedPointer<PluginPopupPrivate> d;
};
}
