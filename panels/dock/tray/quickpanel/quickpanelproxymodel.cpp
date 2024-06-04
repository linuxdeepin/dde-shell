// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "quickpanelproxymodel.h"

#include <QDebug>
#include <DConfig>
DCORE_USE_NAMESPACE

namespace dock {
static const int ProxyRole = Qt::UserRole + 10;
QuickPanelProxyModel::QuickPanelProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    updateQuickPluginsOrder();
    sort(0);
}

QString QuickPanelProxyModel::getTitle(const QString &pluginName) const
{
    return surfaceValue(pluginName, "pluginId").toString();
}

QVariant QuickPanelProxyModel::data(const QModelIndex &index, int role) const
{
    const auto sourceIndex = mapToSource(index);
    switch (role) {
    case ProxyRole + 1:
        return surfaceName(sourceIndex);
    case ProxyRole + 2:
        return surfaceType(sourceIndex);
    case ProxyRole + 3:
        return QVariant::fromValue(surfaceObject(sourceIndex));
    }
    return {};
}

QHash<int, QByteArray> QuickPanelProxyModel::roleNames() const
{
    const QHash<int, QByteArray> roles {
        {ProxyRole + 1, "pluginName"}, // plugin's id.
        {ProxyRole + 2, "type"}, // layout's type. (1, signal), (2, multi), (4, full)
        {ProxyRole + 3, "surface"}, // surface item.
    };
    return roles;
}

bool QuickPanelProxyModel::lessThan(const QModelIndex &sourceLeft, const QModelIndex &sourceRight) const
{
    auto leftOrder = pluginOrder(sourceLeft);
    auto rightOrder = pluginOrder(sourceRight);

    return leftOrder < rightOrder;
}

bool QuickPanelProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    const auto index = this->sourceModel()->index(sourceRow, 0, sourceParent);
    if (!index.isValid())
        return false;
    const auto &name = surfaceName(index);
    return !m_hideInPanelPlugins.contains(name);
}

void QuickPanelProxyModel::updateQuickPluginsOrder()
{
    QScopedPointer<DConfig> dconfig(DConfig::create("org.deepin.ds.dock", "org.deepin.ds.dock.quick-panel"));
    m_quickPlugins = dconfig->value("quickPluginsOrder").toStringList();
    m_hideInPanelPlugins = dconfig->value("hiddenQuickPlugins").toStringList();
    qDebug() << "Fetched QuickPanel's orders by DConfig,"
             << "order list size:" << m_quickPlugins.size()
             << "hide size:" <<m_hideInPanelPlugins.size();
    invalidate();
}

void QuickPanelProxyModel::watchingCountChanged()
{
    static const struct {
        const char *signalName;
        const char *slotName;
    } connectionTable[] = {
                           { SIGNAL(rowsInserted(QModelIndex,int,int)), SLOT(updateTraySurfaceItem()) },
                           { SIGNAL(rowsRemoved(QModelIndex,int,int)), SLOT(updateTraySurfaceItem()) },
                           };

    for (const auto &c : connectionTable) {
        disconnect(m_trayPluginModel, c.signalName, this, c.slotName);
    }
    for (const auto &c : connectionTable) {
        connect(m_trayPluginModel, c.signalName, this, c.slotName);
    }
}

int QuickPanelProxyModel::pluginOrder(const QModelIndex &index) const
{
    const auto name = surfaceName(index);
    auto ret = m_quickPlugins.indexOf(name);
    auto order = surfaceOrder(index);
    if (order >= 0) {
        ret = order;
    }
    auto type = surfaceType(index);
    const QMap<int, int> OrderOffset {
        {1, 2000},
        {2, 1000},
        {4, 4000},
    };
    ret += OrderOffset.value(type);

    return ret;
}

int QuickPanelProxyModel::surfaceType(const QModelIndex &index) const
{
    // Quick_Panel_Single = 0x40, Quick_Panel_Multi = 0x80, Quick_Panel_Full = 0x100
    auto flags = surfaceValue(index, "pluginFlags").toInt();
    if (flags & 0x100)
        return 4;
    if (flags & 0x80)
        return 2;
    return 1;
}

int QuickPanelProxyModel::surfaceOrder(const QModelIndex &index) const
{
    return surfaceValue(index, "order").toInt();
}

QString QuickPanelProxyModel::surfaceName(const QModelIndex &index) const
{
    return surfaceValue(index, "itemKey").toString();
}

QVariant QuickPanelProxyModel::surfaceValue(const QModelIndex &index, const QByteArray &roleName) const
{
    if (auto modelData = surfaceObject(index))
        return modelData->property(roleName);

    return {};
}

QVariant QuickPanelProxyModel::surfaceValue(const QString &pluginName, const QByteArray &roleName) const
{
    const auto targetModel = surfaceModel();
    if (!targetModel)
        return {};
    for (int i = 0; i < targetModel->rowCount(); i++) {
        const auto index = targetModel->index(i, 0);
        const auto name = surfaceName(index);
        if (name == pluginName) {
            if (roleName.isEmpty())
                return QVariant::fromValue(surfaceObject(index));
            return surfaceValue(index, roleName);
        }
    }
    return {};
}

QVariant QuickPanelProxyModel::surfaceValue(const QString &pluginName) const
{
    return surfaceValue(pluginName, {});
}

QObject *QuickPanelProxyModel::surfaceObject(const QModelIndex &index) const
{
    const auto modelDataRole = roleByName("shellSurface");
    if (modelDataRole >= 0)
        return surfaceModel()->data(index, modelDataRole).value<QObject *>();

    return nullptr;
}

int QuickPanelProxyModel::roleByName(const QByteArray &roleName) const
{
    if (!surfaceModel())
        return -1;
    const auto roleNames = surfaceModel()->roleNames();
    return roleNames.key(roleName, -1);
}

QAbstractListModel *QuickPanelProxyModel::surfaceModel() const
{
    return qobject_cast<QAbstractListModel *>(sourceModel());
}

void QuickPanelProxyModel::updateTraySurfaceItem()
{
    emit traySurfaceItemChanged();
}

void QuickPanelProxyModel::classBegin()
{
}

void QuickPanelProxyModel::componentComplete()
{
    updateTraySurfaceItem();
}

QObject *QuickPanelProxyModel::traySurfaceItem() const
{
    if (m_trayItemPluginName.isEmpty())
        return nullptr;

    const auto targetModel = m_trayPluginModel;
    if (!targetModel)
        return nullptr;

    qDebug() << "traySurfaceItem's model count" << m_trayPluginModel->rowCount();
    const auto roleNames = targetModel->roleNames();
    const auto modelDataRole = roleNames.key("shellSurface", -1);
    if (modelDataRole < 0)
        return nullptr;
    for (int i = 0; i < targetModel->rowCount(); i++) {
        const auto index = targetModel->index(i, 0);
        const auto item = index.data(modelDataRole).value<QObject *>();
        if (!item)
            return nullptr;
        const auto name = item->property("itemKey").toString();
        if (name == m_trayItemPluginName)
            return item;
    }
    return nullptr;
}

QString QuickPanelProxyModel::trayItemPluginName() const
{
    return m_trayItemPluginName;
}

void QuickPanelProxyModel::setTrayItemPluginName(const QString &newTrayItemPluginName)
{
    if (m_trayItemPluginName == newTrayItemPluginName)
        return;
    m_trayItemPluginName = newTrayItemPluginName;
    emit trayItemPluginNameChanged();
    updateTraySurfaceItem();
}

QAbstractItemModel *QuickPanelProxyModel::trayPluginModel() const
{
    return m_trayPluginModel;
}

void QuickPanelProxyModel::setTrayPluginModel(QAbstractItemModel *newTrayPluginModel)
{
    if (m_trayPluginModel == newTrayPluginModel)
        return;
    m_trayPluginModel = newTrayPluginModel;
    watchingCountChanged();
    emit trayPluginModelChanged();
}

}
