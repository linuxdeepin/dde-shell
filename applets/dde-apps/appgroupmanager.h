// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QHash>
#include <QObject>
#include <QPointer>
#include <DConfig>
#include <QTimer>
#include <QQmlEngine>

#include <QStandardItemModel>

class ItemsPage;
Q_MOC_INCLUDE("itemspage.h")

namespace apps {
// To make it easier to access in QML
struct ItemPosition
{
    Q_GADGET
    int m_groupId;
    int m_pageNumber;
    int m_positionInPage;
    Q_PROPERTY(int group MEMBER m_groupId)
    Q_PROPERTY(int page MEMBER m_pageNumber)
    Q_PROPERTY(int pos MEMBER m_positionInPage)
public:
    explicit ItemPosition(int group = -1, int page = -1, int pos = -1) : m_groupId(group), m_pageNumber(page), m_positionInPage(pos) {}
    int group() const { return m_groupId; };
    int page() const { return m_pageNumber; };
    int pos() const { return m_positionInPage; };
};

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
    enum DndOperation {
        DndPrepend = -1,
        DndJoin = 0,
        DndAppend = 1
    };
    Q_ENUM(DndOperation)
    explicit AppGroupManager(QAbstractItemModel *referenceModel, QObject *parent = nullptr);
    AppGroupManager(QAbstractItemModel *referenceModel, Dtk::Core::DConfig *config, QObject *parent);

    QHash<int, QByteArray> roleNames() const override;
    Q_INVOKABLE QVariantMap itemPosition(const QString &id);
    Q_INVOKABLE QVariantMap folderInfo(const QString &id);
    Q_INVOKABLE int pageCount(int folderId = 0);
    Q_INVOKABLE void updateFolderName(int folderId, const QString &name);
    Q_INVOKABLE int createEmptyPage(int folderId = 0);
    Q_INVOKABLE void removeEmptyPages();
    Q_INVOKABLE void rearrange(const QString &dragId, const QString &dropId, int operation, int pageHint = -1);
    Q_INVOKABLE void bringToFront(const QString &id);

    Q_INVOKABLE ItemPosition findItem(const QString &appId, int folderId = -1);
    Q_INVOKABLE void appendItemToGroup(const QString &appId, int groupId);
    Q_INVOKABLE bool removeItemFromGroup(const QString &appId, int groupId);
    QModelIndex groupIndexById(int groupId);
    AppGroup * group(int groupId);
    AppGroup * group(QModelIndex idx);
    Q_INVOKABLE ItemsPage * groupPages(int groupId);
    Q_INVOKABLE void commitRearrangeOperation(const QString & dragId, const QString & dropId, DndOperation operation, int pageHint = -1);

    static QVariantList fromListOfStringList(const QList<QStringList> & list);

signals:
    void arrangementChanged();

private slots:
    void onReferenceModelChanged();
    void scheduleReferenceSync() { m_referenceSyncTimer.start(); }

private:
    void notifyArrangementChanged();

    void launchpadArrangementConfigMigration();
    void loadAppGroupInfo();
    void saveAppGroupInfo();
    QString assignGroupId() const;

    AppGroup * appendGroup(int groupId, QString groupName, const QList<QStringList> &appItemIDs);
    AppGroup * appendGroup(QString groupId, QString groupName, const QList<QStringList> &appItemIDs);
    void removeGroup(int groupId);

private:
    QPointer<QAbstractItemModel> m_referenceModel;
    Dtk::Core::DConfig *m_config;
    QTimer m_referenceSyncTimer;
};
}
