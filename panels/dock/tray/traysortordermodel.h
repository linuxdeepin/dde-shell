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
    Q_PROPERTY(bool actionsAlwaysVisible MEMBER m_actionsAlwaysVisible NOTIFY actionsAlwaysVisibleChanged)
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

    enum VisualSections {
        DockTraySection,
        StashedSection
    };
    Q_ENUM(VisualSections)

    explicit TraySortOrderModel(QObject *parent = nullptr);
    ~TraySortOrderModel();

    Q_INVOKABLE bool dropToStashTray(const QString & draggedSurfaceId, int dropVisualIndex, bool isBefore);
    Q_INVOKABLE bool dropToDockTray(const QString & draggedSurfaceId, int dropVisualIndex, bool isBefore);
    Q_INVOKABLE void setSurfaceVisible(const QString & surfaceId, bool visible);

signals:
    void collapsedChanged(bool);
    void actionsAlwaysVisibleChanged(bool);
    void visualItemCountChanged(int);
    void availableSurfacesChanged(const QList<QVariantMap> &);

private:
    int m_visualItemCount = 0;
    bool m_collapsed = false;
    bool m_actionsAlwaysVisible = false;
    // this is for the plugins that currently available.
    QList<QVariantMap> m_availableSurfaces;
    // these are the sort order data source, it might contain items that are no longer existed.
    QStringList m_stashedIds;
    QStringList m_collapsableIds;
    QStringList m_pinnedIds;
    QStringList m_fixedIds;
    // surface IDs that should be invisible/hidden from the tray area.
    QStringList m_hiddenIds;

    QStandardItem * findItemByVisualIndex(int visualIndex, VisualSections visualSection) const;
    QStringList * getSection(const QString & sectionType);
    QString findSection(const QString & surfaceId, const QString & fallback);
    void registerToSection(const QString & surfaceId, const QString & sectionType);
    QStandardItem * createTrayItem(const QString & name, const QString & sectionType, const QString & delegateType);
    void updateVisualIndexes();
    void registerSurfaceId(const QString & name, const QString & delegateType);
    void loadDataFromDConfig();
    void saveDataToDConfig();

private slots:
    void onAvailableSurfacesChanged();
};

}
