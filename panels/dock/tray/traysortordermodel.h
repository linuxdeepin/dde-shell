// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractItemModel>
#include <QQmlEngine>

namespace Dtk {
namespace Core {
class DConfig;
}}

namespace docktray {

struct Item {
    QString surfaceId;
    bool visible;
    QString sectionType;
    QString delegateType;
    QStringList forbiddenSections;
    bool isForceDock;
};

class TraySortOrderModel : public QAbstractItemModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(int rowCount READ rowCount NOTIFY rowCountChanged)
    Q_PROPERTY(bool collapsed MEMBER m_collapsed NOTIFY collapsedChanged)
    Q_PROPERTY(bool isCollapsing MEMBER m_isCollapsing NOTIFY isCollapsingChanged)
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
        DelegateTypeRole,
        // this tray item cannot be drop (or moved in any form) to the given sections
        ForbiddenSectionsRole,
        IsForceDockRole,
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

    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    Q_INVOKABLE bool move(const QString &draggedSurfaceId, const QString &dropOnSurfaceId, bool isBefore);
    Q_INVOKABLE void setSurfaceVisible(const QString & surfaceId, bool visible);

signals:
    void rowCountChanged();
    void collapsedChanged(bool);
    void isCollapsingChanged(bool);
    void actionsAlwaysVisibleChanged(bool);
    void visualItemCountChanged(int);
    void availableSurfacesChanged(const QList<QVariantMap> &);

private:
    QHash<int, QByteArray> m_defaultRoleNames;
    QVector<Item> m_list;
    bool m_collapsed = false;
    bool m_isCollapsing = false;
    bool m_actionsAlwaysVisible = false;
    int m_visibleStashed = 0;
    std::unique_ptr<Dtk::Core::DConfig> m_dconfig;
    // this is for the plugins that currently available.
    QList<QVariantMap> m_availableSurfaces;
    // these are the sort order data source, it might contain items that are no longer existed.
    QStringList m_stashedIds;
    QStringList m_collapsableIds;
    QStringList m_pinnedIds;
    QStringList m_fixedIds;
    // surface IDs that should be invisible/hidden from the tray area.
    QStringList m_hiddenIds;

    QVector<Item>::iterator findItem(const QString &surfaceId);
    QVector<Item>::const_iterator findInsertionPosInSection(const QString &surfaceId, const QString &sectionType,
                                                            QVector<Item>::const_iterator sectionBegin, QVector<Item>::const_iterator sectionEnd) const;
    QVector<Item>::const_iterator findInsertionPos(const QString &surfaceId, const QString &sectionType) const;
    QStringList * getSection(const QString & sectionType);
    QString findSection(const QString & surfaceId, const QString & fallback, const QStringList & forbiddenSections, bool isForceDock);
    void registerToSection(const QString & surfaceId, const QString & sectionType);
    void createTrayItem(const QString & name, const QString & sectionType,
                                  const QString & delegateType, const QStringList & forbiddenSections = {}, bool isForceDock = false);
    void updateStashPlaceholderVisible();
    void updateShowStashActionVisible();
    std::tuple<bool, QString> moveAux(const QString &draggedSurfaceId, const QString &dropOnSurfaceId, bool isBefore);
    QString registerSurfaceId(const QVariantMap &surfaceData);
    bool getSurfaceVisible(const QString &surfaceId, const QString &sectionType);
    bool getStashPlaceholderVisible();
    bool getShowStashActionVisible();
    void loadDataFromDConfig();
    void saveDataToDConfig();

private slots:
    void onAvailableSurfacesChanged();
    void updateVisibilities();
};

}
