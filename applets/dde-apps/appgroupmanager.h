// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QHash>
#include <QObject>
#include <DConfig>
#include <QTimer>

#include <QStandardItemModel>
#include <tuple>

class ItemsPage;
namespace apps {
class AMAppItemModel;
class AppGroup;
/*! \brief AppGroupManager is a interface to manager all groups.
 *
 *  The life cycle of all groups and the appitems of the group are manager by this class.
 */
class AppGroupManager : public QStandardItemModel
{
    Q_OBJECT

public:
    enum Roles {
        GroupIdRole = Qt::UserRole + 1,
        GroupItemsPerPageRole,
        ExtendRole = 0x1000,
    };
    explicit AppGroupManager(AMAppItemModel * referenceModel, QObject* parent = nullptr);

    QVariant data(const QModelIndex &index, int role = GroupIdRole) const override;

    std::tuple<int, int, int> findItem(const QString &appId, int folderId = -1);
    void appendItemToGroup(const QString &appId, int groupId);
    bool removeItemFromGroup(const QString &appId, int groupId);
    QModelIndex groupIndexById(int groupId);
    AppGroup * group(int groupId);
    AppGroup * group(QModelIndex idx);
    void appendAppToGroup(const QString &appId, int groupId);

    static QVariantList fromListOfStringList(const QList<QStringList> & list);

private:
    void onReferenceModelChanged();

    void loadAppGroupInfo();
    void saveAppGroupInfo();
    QString assignGroupId() const;

private:
    bool m_appGroupInitialized;
    AMAppItemModel * m_referenceModel;
    QTimer* m_dumpTimer;
    Dtk::Core::DConfig *m_config;
};
}
