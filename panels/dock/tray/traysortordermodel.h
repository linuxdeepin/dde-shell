// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QStandardItemModel>
#include <QQmlEngine>

namespace docktray {

class TraySortOrderModel : public QStandardItemModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(int visualItemCount MEMBER m_visualItemCount NOTIFY visualItemCountChanged)
    Q_PROPERTY(bool collapsed MEMBER m_collapsed NOTIFY collapsedChanged)
    Q_PROPERTY(QList<QVariantMap> availableSurfaces MEMBER m_availableSurfaces NOTIFY availableSurfacesChanged)
public:
    // enum SectionTypes {
    //     TrayAction,
    //     Stashed,
    //     Collapsable,
    //     Pinned,
    //     Fixed
    // };
    // Q_ENUM(SectionTypes)

    enum Roles {
        SurfaceIdRole = Qt::UserRole, // actually "pluginId::itemKey" or an internal one.
        VisibilityRole,
        SectionTypeRole,
        VisualIndexRole,
        DelegateTypeRole,
        ModelExtendedRole = 0x1000
    };
    Q_ENUM(Roles)

    explicit TraySortOrderModel(QObject *parent = nullptr);
    ~TraySortOrderModel();

signals:
    void collapsedChanged(bool);
    void visualItemCountChanged(int);
    void availableSurfacesChanged(const QList<QVariantMap> &);

private:
    int m_visualItemCount = 0;
    bool m_collapsed = false;
    // this is for the plugins that currently available.
    QList<QVariantMap> m_availableSurfaces;
    // these are the sort order data source, it might contain items that are no longer existed.
    QStringList m_stashedIds;
    QStringList m_collapsableIds;
    QStringList m_pinnedIds;
    QStringList m_fixedIds;

    QString findSection(const QString & surfaceId, const QString & fallback);
    void registerToSection(const QString & surfaceId, const QString & sectionType);
    QStandardItem * createTrayItem(const QString & name, const QString & sectionType, const QString & delegateType);
    void updateVisualIndexes();
    void registerSurfaceId(const QString & name, const QString & delegateType);

private slots:
    void onAvailableSurfacesChanged();
};

}
