// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractItemModel>
#include <QObject>
#include <QPointer>
#include <QVariantList>
#include <QVariantMap>

namespace dock {

class TaskOverflowController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(qreal availableExtent READ availableExtent WRITE setAvailableExtent)
    Q_PROPERTY(bool minimumReached READ minimumReached WRITE setMinimumReached)
    Q_PROPERTY(qreal fallbackItemExtent READ fallbackItemExtent WRITE setFallbackItemExtent NOTIFY fallbackItemExtentChanged)
    Q_PROPERTY(qreal spacing READ spacing WRITE setSpacing NOTIFY spacingChanged)
    Q_PROPERTY(QObject *visualModel READ visualModel WRITE setVisualModel)
    Q_PROPERTY(QAbstractItemModel *dataModel READ dataModel WRITE setDataModel)
    Q_PROPERTY(int visibleItemCount READ visibleItemCount NOTIFY visibleItemCountChanged)
    Q_PROPERTY(QVariantList popupItems READ popupItems NOTIFY popupItemsChanged)

public:
    explicit TaskOverflowController(QObject *parent = nullptr);

    bool isEnabled() const
    {
        return m_enabled;
    }
    void setEnabled(bool enabled);

    qreal availableExtent() const
    {
        return m_availableExtent;
    }
    void setAvailableExtent(qreal availableExtent);

    bool minimumReached() const
    {
        return m_minimumReached;
    }
    void setMinimumReached(bool minimumReached);

    qreal fallbackItemExtent() const
    {
        return m_fallbackItemExtent;
    }
    void setFallbackItemExtent(qreal fallbackItemExtent);

    qreal spacing() const
    {
        return m_spacing;
    }
    void setSpacing(qreal spacing);

    QObject *visualModel() const
    {
        return m_visualModel;
    }
    void setVisualModel(QObject *visualModel);

    QAbstractItemModel *dataModel() const
    {
        return m_dataModel;
    }
    void setDataModel(QAbstractItemModel *dataModel);

    int visibleItemCount() const
    {
        return m_visibleItemCount;
    }
    QVariantList popupItems() const
    {
        return m_popupItems;
    }

    Q_INVOKABLE void scheduleSync();

Q_SIGNALS:
    void enabledChanged();
    void fallbackItemExtentChanged();
    void spacingChanged();
    void visibleItemCountChanged();
    void popupItemsChanged();

private Q_SLOTS:
    void onDataModelDestroyed();

private:
    void sync();
    int modelCount() const;
    QModelIndex modelIndexAt(int visualIndex) const;
    QVariant dataForRole(const QModelIndex &index, const QByteArray &roleName) const;
    QVariantMap entryAt(int visualIndex) const;
    bool syncEnabledState(int totalCount, qreal fullExtent);

    void connectDataModelSignals();
    void disconnectDataModelSignals();

    void setVisibleItemCount(int visibleItemCount);
    void setPopupItems(const QVariantList &items);

    bool m_enabled = true;
    qreal m_availableExtent = 0;
    bool m_minimumReached = false;
    qreal m_fallbackItemExtent = 1;
    qreal m_spacing = 0;
    QPointer<QObject> m_visualModel;
    QPointer<QAbstractItemModel> m_dataModel;
    bool m_latched = false;
    int m_visibleItemCount = 0;
    QVariantList m_popupItems;
    bool m_syncPending = false;
};

}
