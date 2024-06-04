// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QtQml/qqml.h>
#include <QSortFilterProxyModel>
#include <QQmlParserStatus>

namespace dock {

class QuickPanelProxyModel : public QSortFilterProxyModel, public QQmlParserStatus
{
    Q_OBJECT
    Q_PROPERTY(QString trayItemPluginName READ trayItemPluginName WRITE setTrayItemPluginName NOTIFY trayItemPluginNameChanged FINAL)
    Q_PROPERTY(QObject* traySurfaceItem READ traySurfaceItem NOTIFY traySurfaceItemChanged FINAL)
    Q_PROPERTY(QAbstractItemModel* trayPluginModel READ trayPluginModel WRITE setTrayPluginModel NOTIFY trayPluginModelChanged FINAL)
    QML_NAMED_ELEMENT(QuickPanelProxyModel)
    Q_INTERFACES(QQmlParserStatus)
public:
    explicit QuickPanelProxyModel(QObject *parent = nullptr);

    Q_INVOKABLE QString getTitle(const QString &pluginName) const;

    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QObject *traySurfaceItem() const;

    QString trayItemPluginName() const;
    void setTrayItemPluginName(const QString &newTrayItemPluginName);

    QAbstractItemModel *trayPluginModel() const;
    void setTrayPluginModel(QAbstractItemModel *newTrayPluginModel);

signals:
    void traySurfaceItemChanged();
    void trayItemPluginNameChanged();

    void trayPluginModelChanged();

public:
    void classBegin() override;
    void componentComplete() override;

protected:
    bool lessThan(const QModelIndex &sourceLeft, const QModelIndex &sourceRight) const override;
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    void updateQuickPluginsOrder();
    void watchingCountChanged();

    int pluginOrder(const QModelIndex &index) const;
    int surfaceType(const QModelIndex &index) const;
    int surfaceOrder(const QModelIndex &index) const;
    QString surfaceName(const QModelIndex &index) const;
    QVariant surfaceValue(const QModelIndex &index, const QByteArray &roleName) const;
    QVariant surfaceValue(const QString &pluginName, const QByteArray &roleName) const;
    QVariant surfaceValue(const QString &pluginName) const;
    QObject *surfaceObject(const QModelIndex &index) const;
    int roleByName(const QByteArray &roleName) const;
    QAbstractListModel *surfaceModel() const;
private slots:
    void updateTraySurfaceItem();

private:
    QStringList m_quickPlugins;
    QStringList m_hideInPanelPlugins;
    QString m_trayItemPluginName;
    QAbstractItemModel *m_trayPluginModel = nullptr;
};

}
