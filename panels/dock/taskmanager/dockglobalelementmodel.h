// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "abstracttaskmanagerinterface.h"
#include "dockcombinemodel.h"
#include <QAbstractProxyModel>
#include <QPointer>
#include <tuple>

namespace dock
{
class FolderPresentation;

class DockGlobalElementModel : public QAbstractListModel, public AbstractTaskManagerInterface
{
    Q_OBJECT
public:
    explicit DockGlobalElementModel(QAbstractItemModel *appsModel, DockCombineModel *activeAppModel, QObject *parent = nullptr);

    void setApplicationFolderModel(QAbstractItemModel *model);
    bool containsApplicationFolder(const QString &id) const;
    QHash<int, QByteArray> roleNames() const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;

    inline int mapToSourceModelRole(QAbstractItemModel *model, int role) const;

    void requestActivate(const QModelIndex &index) const override;
    void requestNewInstance(const QModelIndex &index, const QString &action) const override;

    void requestOpenUrls(const QModelIndex &index, const QList<QUrl> &urls) const override;
    void requestClose(const QModelIndex &index, bool force = false) const override;
    void requestUpdateWindowIconGeometry(const QModelIndex &index, const QRect &geometry, QObject *delegate = nullptr) const override;

    void requestWindowsView(const QModelIndexList &indexes) const override;

    void moveItem(int from, int to);

public slots:
    void initDockedElements(bool unused);
    void syncApplicationFolders();

private:
    // Application id, source model, cached row, and stable window id (for running items).
    // 目前 Item的 保存的 四种属性为： 应用id，源模型，缓存行号，稳定的窗口id（用于运行中的项目）
    using Item = std::tuple<QString, QAbstractItemModel *, int, QString>;
    Item makeItem(const QString &id, QAbstractItemModel *model, int row) const;
    QModelIndex sourceIndex(const Item &item) const;
    void scheduleWindowSync();
    void syncWindows();
    QModelIndex applicationFolderIndex(const QString &id) const;
    QString applicationFolderName(const QString &id) const;
    QVariantList applicationFolderContents(const QString &id) const;
    void loadDockedElements();
    void ensureFolderPresentation(const QString &id);
    // Repoint a folder pin at the new id its provider resolved after a rename,
    // so the persisted reference stays usable instead of pointing at a dead path.
    void rebindFolderElement(FolderPresentation *presentation, const QString &newId);
    // Drop a folder pin whose directory was deleted, so the dock does not keep
    // an entry that can never show contents.
    void dropFolderElement(FolderPresentation *presentation);
    QString getMenus(const QModelIndex &index) const;
    void groupItemsByApp();

private:
    QList<Item> m_data;
    bool m_windowSyncPending = false;

    // type, id
    QList<std::tuple<QString, QString>> m_dockedElements;
    QPointer<QAbstractItemModel> m_applicationFolderModel;
    QHash<QString, FolderPresentation *> m_folderPresentations;
    QAbstractItemModel *m_appsModel;
    DockCombineModel *m_activeAppModel;
};
}
