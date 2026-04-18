// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dockglobalelementmodel.h"
#include "applicationinterface.h"
#include "globals.h"
#include "taskmanager.h"
#include "taskmanagersettings.h"

#include <QAbstractListModel>
#include <QDBusConnection>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QMetaObject>
#include <QMimeDatabase>
#include <QProcess>
#include <QUrl>

Q_LOGGING_CATEGORY(dockGlobalElementModelLog, "org.deepin.dde.shell.dock.taskmanager.dockglobalelementmodel")

#include <algorithm>
#include <tuple>

namespace dock
{
static QPair<QString, QString> splitDockElement(const QString &dockElement)
{
    const int separatorIndex = dockElement.indexOf(QLatin1Char('/'));
    if (separatorIndex <= 0) {
        return {};
    }

    return {dockElement.left(separatorIndex), dockElement.mid(separatorIndex + 1)};
}

static QString displayNameForPath(const QString &path)
{
    QFileInfo fileInfo(path);
    QString name = fileInfo.fileName();
    if (name.isEmpty()) {
        name = fileInfo.absoluteFilePath();
    }
    if (name.isEmpty()) {
        name = path;
    }
    return name;
}

static QString fileIconName(const QFileInfo &fileInfo)
{
    if (fileInfo.isDir()) {
        return QStringLiteral("folder");
    }

    static QMimeDatabase mimeDatabase;
    const auto mimeType = mimeDatabase.mimeTypeForFile(fileInfo, QMimeDatabase::MatchDefault);
    if (!mimeType.iconName().isEmpty()) {
        return mimeType.iconName();
    }
    if (!mimeType.genericIconName().isEmpty()) {
        return mimeType.genericIconName();
    }

    return QStringLiteral("text-x-generic");
}

static QModelIndex findIndexByRole(QAbstractItemModel *model, int role, const QString &value)
{
    if (!model || value.isEmpty()) {
        return {};
    }

    return model->match(model->index(0, 0), role, value, 1, Qt::MatchExactly).value(0);
}

static int modelRole(QAbstractItemModel *model, const QByteArray &roleName, int fallbackRole)
{
    if (!model) {
        return fallbackRole;
    }

    return model->roleNames().key(roleName, fallbackRole);
}

static QModelIndex findIndexByNamedRole(QAbstractItemModel *model,
                                        const QByteArray &roleName,
                                        const QString &value,
                                        int fallbackRole)
{
    return findIndexByRole(model, modelRole(model, roleName, fallbackRole), value);
}

static bool isLauncherFolderId(const QString &launcherId)
{
    return launcherId.startsWith(QStringLiteral("internal/folder/")) ||
           launcherId.startsWith(QStringLiteral("internal/folders/")) ||
           launcherId.startsWith(QStringLiteral("internal/group/"));
}

static QString alternateLauncherFolderId(const QString &launcherId)
{
    const QString singularPrefix = QStringLiteral("internal/folder/");
    const QString pluralPrefix = QStringLiteral("internal/folders/");
    const QString legacyPrefix = QStringLiteral("internal/group/");

    if (launcherId.startsWith(singularPrefix)) {
        return pluralPrefix + launcherId.mid(singularPrefix.size());
    }

    if (launcherId.startsWith(pluralPrefix)) {
        return singularPrefix + launcherId.mid(pluralPrefix.size());
    }

    if (launcherId.startsWith(legacyPrefix)) {
        return pluralPrefix + launcherId.mid(legacyPrefix.size());
    }

    return {};
}

static QString resolveLauncherGroupId(QAbstractItemModel *groupModel, const QString &groupId)
{
    if (!groupModel || !isLauncherFolderId(groupId)) {
        return groupId;
    }

    const QString suffix = groupId.section(QLatin1Char('/'), -1);
    const QStringList candidates = {
        groupId,
        alternateLauncherFolderId(groupId),
        QStringLiteral("internal/folders/%1").arg(suffix),
        QStringLiteral("internal/folder/%1").arg(suffix),
        QStringLiteral("internal/group/%1").arg(suffix),
        suffix
    };

    for (const QString &candidate : candidates) {
        if (!candidate.isEmpty() &&
            findIndexByNamedRole(groupModel, MODEL_DESKTOPID, candidate, TaskManager::DesktopIdRole).isValid()) {
            return candidate;
        }
    }

    return groupId;
}

static QStringList invokeLauncherGroupItems(QObject *groupModel, const QString &groupId)
{
    if (!groupModel || groupId.isEmpty()) {
        return {};
    }

    const QString resolvedGroupId = resolveLauncherGroupId(qobject_cast<QAbstractItemModel *>(groupModel), groupId);
    QStringList items;
    QMetaObject::invokeMethod(groupModel,
                              "groupItems",
                              Q_RETURN_ARG(QStringList, items),
                              Q_ARG(QString, resolvedGroupId));
    return items;
}

static QStringList previewIconsForDirectory(const QString &path, int limit = 4)
{
    QStringList iconNames;

    QDir directory(path);
    const QFileInfoList fileInfos = directory.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot,
                                                            QDir::DirsFirst | QDir::Name | QDir::IgnoreCase);
    for (const QFileInfo &fileInfo : fileInfos) {
        iconNames.append(fileIconName(fileInfo));
        if (iconNames.size() >= limit) {
            break;
        }
    }

    return iconNames;
}

DockGlobalElementModel::DockGlobalElementModel(QAbstractItemModel *appsModel,
                                               DockCombineModel *activeAppModel,
                                               QAbstractItemModel *groupModel,
                                               QObject *parent)
    : QAbstractListModel(parent)
    , AbstractTaskManagerInterface(nullptr)
    , m_appsModel(appsModel)
    , m_activeAppModel(activeAppModel)
    , m_groupModel(groupModel)
{
    connect(TaskManagerSettings::instance(), &TaskManagerSettings::dockedElementsChanged, this, &DockGlobalElementModel::loadDockedElements);
    connect(TaskManagerSettings::instance(), &TaskManagerSettings::windowSplitChanged, this, &DockGlobalElementModel::groupItemsByApp);
    if (m_groupModel) {
        auto refreshDockedElements = [this]() {
            QMetaObject::invokeMethod(this, &DockGlobalElementModel::loadDockedElements, Qt::QueuedConnection);
        };
        connect(m_groupModel, &QAbstractItemModel::dataChanged, this, refreshDockedElements, Qt::QueuedConnection);
        connect(m_groupModel, &QAbstractItemModel::modelReset, this, refreshDockedElements, Qt::QueuedConnection);
        connect(m_groupModel, &QAbstractItemModel::rowsInserted, this, refreshDockedElements, Qt::QueuedConnection);
        connect(m_groupModel, &QAbstractItemModel::rowsRemoved, this, refreshDockedElements, Qt::QueuedConnection);
    }

    connect(
        m_appsModel,
        &QAbstractItemModel::rowsRemoved,
        this,
        [this](const QModelIndex &parent, int first, int last) {
            Q_UNUSED(parent)
            for (int i = first; i <= last; ++i) {
                auto it = std::find_if(m_data.begin(), m_data.end(), [this, &i](auto data) {
                    return std::get<2>(data) == m_appsModel && std::get<3>(data) == i;
                });
                if (it != m_data.end()) {
                    auto pos = it - m_data.begin();
                    beginRemoveRows(QModelIndex(), pos, pos);
                    m_data.remove(pos);
                    endRemoveRows();
                }
            }
            std::for_each(m_data.begin(), m_data.end(), [this, first, last](auto &data) {
                if (std::get<2>(data) == m_appsModel && std::get<3>(data) >= first) {
                    data = std::make_tuple(std::get<0>(data),
                                           std::get<1>(data),
                                           std::get<2>(data),
                                           std::get<3>(data) - ((last - first) + 1));
                }
            });
        },
        Qt::QueuedConnection);

    connect(
        m_activeAppModel,
        &QAbstractItemModel::rowsInserted,
        this,
        [this](const QModelIndex &parent, int first, int last) {
            Q_UNUSED(parent)
            for (int i = first; i <= last; ++i) {
                auto index = m_activeAppModel->index(i, 0);
                auto desktopId = index.data(TaskManager::DesktopIdRole).toString();

                if (desktopId.isEmpty())
                    continue;
                // 将同一应用的窗口添加到一起
                // Find the first occurrence of this app in m_data (either docked item or existing window)
                auto firstIt = std::find_if(m_data.begin(), m_data.end(), [&desktopId](const auto &data) {
                    return std::get<0>(data) == QStringLiteral("desktop") && std::get<1>(data) == desktopId;
                });

                if (firstIt == m_data.end()) {
                    // No docked item or existing window yet, append to the end
                    beginInsertRows(QModelIndex(), m_data.size(), m_data.size());
                    m_data.append(std::make_tuple(QStringLiteral("desktop"), desktopId, m_activeAppModel, i));
                    endInsertRows();
                    continue;
                }

                // If the first occurrence still comes from m_appsModel, this is the first window:
                // reuse the docked position and turn it into a running window.
                if (std::get<2>(*firstIt) == m_appsModel) {
                    *firstIt = std::make_tuple(QStringLiteral("desktop"), desktopId, m_activeAppModel, i);
                    auto pIndex = this->index(firstIt - m_data.begin(), 0);
                    Q_EMIT dataChanged(pIndex,
                                       pIndex,
                                       {TaskManager::ActiveRole,
                                        TaskManager::AttentionRole,
                                        TaskManager::WindowsRole,
                                        TaskManager::MenusRole,
                                        TaskManager::WinTitleRole});
                    continue;
                }

                // There are already windows for this app: insert the new window
                // right after the last (rightmost) existing one.
                // Search the entire list since windows may not be consecutive after drag reorder.
                auto lastIt = firstIt;
                for (auto it = firstIt + 1; it != m_data.end(); ++it) {
                    if (std::get<0>(*it) == QStringLiteral("desktop") && std::get<1>(*it) == desktopId) {
                        lastIt = it;
                    }
                }

                auto insertRow = (lastIt - m_data.begin()) + 1;
                beginInsertRows(QModelIndex(), insertRow, insertRow);
                m_data.insert(lastIt + 1, std::make_tuple(QStringLiteral("desktop"), desktopId, m_activeAppModel, i));
                endInsertRows();
            }

            std::for_each(m_data.begin(), m_data.end(), [this, first, last](auto &data) {
                if (std::get<2>(data) == m_activeAppModel && std::get<3>(data) > first) {
                    data = std::make_tuple(std::get<0>(data),
                                           std::get<1>(data),
                                           std::get<2>(data),
                                           std::get<3>(data) + ((last - first) + 1));
                }
            });
        },
        Qt::QueuedConnection);

    connect(
        m_activeAppModel,
        &QAbstractItemModel::rowsRemoved,
        this,
        [this](const QModelIndex &parent, int first, int last) {
            Q_UNUSED(parent)

            QList<int> pendingDataChangedRows;

            for (int i = first; i <= last; ++i) {
                auto it = std::find_if(m_data.begin(), m_data.end(), [this, i](auto data) {
                    return std::get<2>(data) == m_activeAppModel && std::get<3>(data) == i;
                });

                if (it == m_data.end()) {
                    qWarning() << "failed to find a running apps on dock" << i;
                    continue;
                }

                auto pos = it - m_data.begin();
                auto type = std::get<0>(*it);
                auto id = std::get<1>(*it);

                auto oit = std::find_if(m_data.constBegin(), m_data.constEnd(), [this, &type, &id, i](auto &data) {
                    return std::get<0>(data) == type &&
                           std::get<1>(data) == id &&
                           std::get<2>(data) == m_activeAppModel &&
                           std::get<3>(data) != i;
                });

                if (type == QStringLiteral("desktop") &&
                    oit == m_data.constEnd() &&
                    m_dockedElements.contains(std::make_tuple(QStringLiteral("desktop"), id))) {
                    auto res = m_appsModel->match(m_appsModel->index(0, 0), TaskManager::DesktopIdRole, id, 1, Qt::MatchExactly);
                    if (res.isEmpty()) {
                        beginRemoveRows(QModelIndex(), pos, pos);
                        m_data.remove(pos);
                        endRemoveRows();
                    } else {
                        auto row = res.first().row();
                        *it = std::make_tuple(QStringLiteral("desktop"), id, m_appsModel, row);
                        // DEFER emitter until internal model shift is done!
                        pendingDataChangedRows.append(pos);
                    }
                } else {
                    beginRemoveRows(QModelIndex(), pos, pos);
                    m_data.remove(pos);
                    endRemoveRows();
                }
            }

            // Adjust remaining row mappings for the active app model BEFORE any outer access
            std::for_each(m_data.begin(), m_data.end(), [this, first, last](auto &data) {
                if (std::get<2>(data) == m_activeAppModel && std::get<3>(data) >= first) {
                    data = std::make_tuple(std::get<0>(data),
                                           std::get<1>(data),
                                           std::get<2>(data),
                                           std::get<3>(data) - ((last - first) + 1));
                }
            });

            // Now it is safe to emit dataChanged for rows that were swapped to docked elements
            for (int pos : pendingDataChangedRows) {
                auto pIndex = this->index(pos, 0);
                Q_EMIT dataChanged(pIndex,
                                   pIndex,
                                   {TaskManager::ActiveRole, TaskManager::AttentionRole, TaskManager::WindowsRole, TaskManager::MenusRole, TaskManager::WinTitleRole});
            }
        },
        Qt::QueuedConnection);

    connect(
        m_activeAppModel,
        &QAbstractItemModel::dataChanged,
        this,
        [this](const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles) {
            int first = topLeft.row(), last = bottomRight.row();
            for (int i = first; i <= last; i++) {
                auto it = std::find_if(m_data.constBegin(), m_data.constEnd(), [this, i](auto data) {
                    return std::get<2>(data) == m_activeAppModel && std::get<3>(data) == i;
                });

                if (it == m_data.end())
                    return;
                auto pos = it - m_data.constBegin();

                auto oldRoles = roles;
                auto desktopId = roles.indexOf(TaskManager::DesktopIdRole);
                auto identifyId = roles.indexOf(TaskManager::IdentityRole);
                if (desktopId != -1 || identifyId != -1) {
                    oldRoles.append(TaskManager::ItemIdRole);
                    oldRoles.append(TaskManager::DockElementRole);
                }
                Q_EMIT dataChanged(index(pos, 0), index(pos, 0), oldRoles);
            }
        },
        Qt::QueuedConnection);

    QMetaObject::invokeMethod(this, &DockGlobalElementModel::loadDockedElements, Qt::QueuedConnection);
}

QHash<int, QByteArray> DockGlobalElementModel::roleNames() const
{
    return {
        {TaskManager::ItemIdRole, MODEL_ITEMID},
        {TaskManager::DockElementRole, "dockElement"},
        {TaskManager::ItemKindRole, "itemKind"},
        {TaskManager::PreviewIconsRole, "previewIcons"},
        {TaskManager::NameRole, MODEL_NAME},
        {TaskManager::IconNameRole, MODEL_ICONNAME},
        {TaskManager::ActiveRole, MODEL_ACTIVE},
        {TaskManager::AttentionRole, MODEL_ATTENTION},
        {TaskManager::MenusRole, MODEL_MENUS},
        {TaskManager::DockedRole, MODEL_DOCKED},
        {TaskManager::WindowsRole, MODEL_WINDOWS},
        {TaskManager::WinTitleRole, MODEL_TITLE},
        {TaskManager::WinIconRole, MODEL_WINICON},
        {TaskManager::WinIdRole, MODEL_WINID},
    };
}

QModelIndex DockGlobalElementModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return createIndex(row, column);
}

QModelIndex DockGlobalElementModel::parent(const QModelIndex &child) const
{
    Q_UNUSED(child)
    return QModelIndex();
}

int DockGlobalElementModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 1;
}

QString DockGlobalElementModel::dockElementForRow(int row) const
{
    if (row < 0 || row >= m_data.size()) {
        return {};
    }

    const auto data = m_data.at(row);
    return QStringLiteral("%1/%2").arg(std::get<0>(data), std::get<1>(data));
}

QString DockGlobalElementModel::displayNameFor(const QString &type, const QString &id) const
{
    if (type == QStringLiteral("group")) {
        const QString resolvedGroupId = resolveLauncherGroupId(m_groupModel, id);
        const QModelIndex groupIndex = findIndexByNamedRole(m_groupModel, MODEL_DESKTOPID, resolvedGroupId, TaskManager::DesktopIdRole);
        QString groupName = groupIndex.data(modelRole(m_groupModel, MODEL_NAME, TaskManager::NameRole)).toString();
        if (groupName.isEmpty() || groupName.startsWith(QStringLiteral("internal/category/"))) {
            groupName = tr("App Group");
        }
        return groupName;
    }

    if (type == QStringLiteral("folder")) {
        return displayNameForPath(id);
    }

    return {};
}

QString DockGlobalElementModel::iconNameFor(const QString &type, const QString &id) const
{
    Q_UNUSED(id)
    if (type == QStringLiteral("group") || type == QStringLiteral("folder")) {
        return QStringLiteral("folder");
    }

    return QString::fromLatin1(DEFAULT_APP_ICONNAME);
}

QStringList DockGlobalElementModel::previewIconsFor(const QString &type, const QString &id) const
{
    if (type == QStringLiteral("group")) {
        QStringList iconNames;
        for (const QString &appId : invokeLauncherGroupItems(m_groupModel, id)) {
            const QModelIndex appIndex = findIndexByNamedRole(m_appsModel, MODEL_DESKTOPID, appId, TaskManager::DesktopIdRole);
            QString iconName = appIndex.data(modelRole(m_appsModel, MODEL_ICONNAME, TaskManager::IconNameRole)).toString();
            if (iconName.isEmpty()) {
                iconName = QString::fromLatin1(DEFAULT_APP_ICONNAME);
            }
            iconNames.append(iconName);
            if (iconNames.size() >= 4) {
                break;
            }
        }
        return iconNames;
    }

    if (type == QStringLiteral("folder")) {
        return previewIconsForDirectory(id);
    }

    return {};
}

void DockGlobalElementModel::initDockedElements(bool unused)
{
    Q_UNUSED(unused);
    loadDockedElements();
}

void DockGlobalElementModel::loadDockedElements()
{
    QList<std::tuple<QString, QString>> newDocked;
    for (auto elementInfo : TaskManagerSettings::instance()->dockedElements()) {
        const auto [type, id] = splitDockElement(elementInfo);
        if (type.isEmpty() || id.isEmpty())
            continue;

        auto tmp = std::make_tuple(type, id);

        QAbstractItemModel *model = nullptr;
        int row = -1;
        if (type == QStringLiteral("desktop")) {
            model = m_appsModel;
            auto res = findIndexByNamedRole(m_appsModel, MODEL_DESKTOPID, id, TaskManager::DesktopIdRole);
            if (!res.isValid())
                continue;
            row = res.row();
        } else if (type == QStringLiteral("group")) {
            const QString resolvedGroupId = resolveLauncherGroupId(m_groupModel, id);
            auto res = findIndexByNamedRole(m_groupModel, MODEL_DESKTOPID, resolvedGroupId, TaskManager::DesktopIdRole);
            if (!res.isValid())
                continue;
        } else if (type != QStringLiteral("folder")) {
            continue;
        }

        newDocked.append(tmp);
        if (m_dockedElements.contains(tmp))
            continue;

        auto isRunning = std::any_of(m_data.constBegin(), m_data.constEnd(), [&type, &id](const auto &data) {
            return std::get<0>(data) == type && std::get<1>(data) == id;
        });

        if (!isRunning) {
            beginInsertRows(QModelIndex(), m_data.size(), m_data.size());
            m_data.append(std::make_tuple(type, id, model, row));
            endInsertRows();
        }
    }

    for (auto it = m_dockedElements.begin(); it < m_dockedElements.end(); ++it) {
        if (newDocked.contains(*it))
            continue;
        auto type = std::get<0>(*it), id = std::get<1>(*it);
        auto dataIt = std::find_if(m_data.begin(), m_data.end(), [this, &type, &id](const auto &data) {
            if (std::get<0>(data) != type || std::get<1>(data) != id) {
                return false;
            }

            if (type == QStringLiteral("desktop")) {
                return std::get<2>(data) == m_appsModel;
            }

            return std::get<2>(data) == nullptr;
        });
        if (dataIt != m_data.end()) {
            auto pos = (dataIt - m_data.begin());
            beginRemoveRows(QModelIndex(), pos, pos);
            m_data.remove(pos);
            endRemoveRows();
        }
    }

    m_dockedElements = newDocked;

    qCDebug(dockGlobalElementModelLog) << "loaded docked elements count:" << m_dockedElements.count() << "appsModel row count:" << m_appsModel->rowCount();

    if (!m_data.isEmpty()) {
        // MenusRole should also be handled here due to it contains the copywriting of docked or undocked
        Q_EMIT dataChanged(index(0, 0),
                           index(m_data.size() - 1, 0),
                           {TaskManager::DockedRole,
                            TaskManager::MenusRole,
                            TaskManager::NameRole,
                            TaskManager::IconNameRole,
                            TaskManager::PreviewIconsRole});
    }
}

QString DockGlobalElementModel::getMenus(const QModelIndex &index) const
{
    auto data = m_data.at(index.row());
    auto type = std::get<0>(data);
    auto id = std::get<1>(data);
    auto model = std::get<2>(data);
    auto row = std::get<3>(data);

    QJsonArray menusArray;
    QString appNameInMenu = index.data(TaskManager::NameRole).toString();
    if (model == m_activeAppModel) {
        // In case a window does not belongs to a known application, use the window title instead
        if (appNameInMenu.isEmpty()) {
            appNameInMenu = index.data(TaskManager::WinTitleRole).toString();
        }
    }
    if (appNameInMenu.isEmpty()) {
        appNameInMenu = tr("Open");
    }
    menusArray.append(QJsonObject{{"id", ""}, {"name", appNameInMenu}});

    if (model) {
        auto actions = model->index(row, 0).data(TaskManager::ActionsRole).toByteArray();
        for (auto action : QJsonDocument::fromJson(actions).array()) {
            menusArray.append(action);
        }
    }

    bool isDocked = m_dockedElements.contains(std::make_tuple(type, id));
    if (type == QStringLiteral("folder")) {
        menusArray.append(QJsonObject{{"id", DOCK_ACTION_OPEN_IN_FILEMANAGER}, {"name", tr("Open in File Manager")}});
    }
    menusArray.append(QJsonObject{{"id", DOCK_ACTION_DOCK}, {"name", isDocked ? tr("Undock") : tr("Dock")}});

    if (model == m_activeAppModel) {
        if (TaskManagerSettings::instance()->isAllowedForceQuit()) {
            menusArray.append(QJsonObject{{"id", DOCK_ACTION_FORCEQUIT}, {"name", tr("Force Quit")}});
        }
        if (TaskManagerSettings::instance()->isWindowSplit()) {
            menusArray.append(QJsonObject{{"id", DOCK_ACTION_CLOSEWINDOW}, {"name", tr("Close this window")}});
        } else {
            menusArray.append(QJsonObject{{"id", DOCK_ACTION_CLOSEALL}, {"name", tr("Close All")}});
        }
    }

    return QJsonDocument(menusArray).toJson();
}

int DockGlobalElementModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_data.size();
}

QVariant DockGlobalElementModel::data(const QModelIndex &index, int role) const
{
    if (index.row() >= m_data.size())
        return {};

    auto data = m_data.at(index.row());
    auto type = std::get<0>(data);
    auto id = std::get<1>(data);
    auto model = std::get<2>(data);
    auto row = std::get<3>(data);
    const QModelIndex sourceIndex = model ? model->index(row, 0) : QModelIndex();

    switch (role) {
    case TaskManager::ItemIdRole:
        return id;
    case TaskManager::DockElementRole:
        return dockElementForRow(index.row());
    case TaskManager::ItemKindRole:
        return type;
    case TaskManager::PreviewIconsRole:
        return previewIconsFor(type, id);
    case TaskManager::WindowsRole: {
        if (model == m_activeAppModel) {
            return QStringList{model->index(row, 0).data(TaskManager::WinIdRole).toString()};
        }
        if (!model) {
            return QStringList{};
        }
        QModelIndex groupIndex = model->index(row, 0);
        return groupIndex.data(TaskManager::WindowsRole).toStringList();
    }
    case TaskManager::ActiveRole:
    case TaskManager::AttentionRole: {
        if (model == m_activeAppModel) {
            return model->index(row, 0).data(role);
        }
        return false;
    }

    case TaskManager::MenusRole: {
        return getMenus(index);
    }
    case TaskManager::NameRole:
        if (!model) {
            return displayNameFor(type, id);
        }
        return sourceIndex.data(role);
    case TaskManager::IconNameRole:
        if (!model) {
            return iconNameFor(type, id);
        }
        return sourceIndex.data(role);
    case TaskManager::DockedRole:
        if (!model) {
            return true;
        }
        return sourceIndex.data(role);
    case TaskManager::ActiveRole:
    case TaskManager::AttentionRole:
        if (!model) {
            return false;
        }
        return sourceIndex.data(role);
    case TaskManager::DesktopIdRole:
        if (!model) {
            return id;
        }
        return sourceIndex.data(role);

    default: {
        if (model) {
            return sourceIndex.data(role);
        }
        return {};
    }
    }

    return {};
}

void DockGlobalElementModel::requestActivate(const QModelIndex &index) const
{
    auto data = m_data.value(index.row());
    auto sourceModel = std::get<2>(data);
    auto sourceRow = std::get<3>(data);

    if (!sourceModel) {
        return;
    }

    if (sourceModel == m_activeAppModel) {
        auto sourceIndex = sourceModel->index(sourceRow, 0);
        m_activeAppModel->requestActivate(sourceIndex);
    } else {
        if (auto interface = dynamic_cast<AbstractTaskManagerInterface*>(sourceModel)) {
            auto sourceIndex = sourceModel->index(sourceRow, 0);
            interface->requestNewInstance(sourceIndex, "");
        }
    }
}

void DockGlobalElementModel::requestNewInstance(const QModelIndex &index, const QString &action) const
{
    auto data = m_data.value(index.row());
    auto type = std::get<0>(data);
    auto id = std::get<1>(data);
    auto sourceModel = std::get<2>(data);
    qDebug(dockGlobalElementModelLog) << "Requesting new instance for index:" << index << "with action:" << action << "type:" << type << "id:" << id;

    // Handle special actions first (for both active and docked apps)
    if (action == DOCK_ACTION_DOCK) {
        TaskManagerSettings::instance()->toggleDockedElement(dockElementForRow(index.row()));
        return;
    } else if (action == DOCK_ACTION_FORCEQUIT) {
        requestClose(index, true);
        return;
    } else if (action == DOCK_ACTION_CLOSEWINDOW || action == DOCK_ACTION_CLOSEALL) {
        requestClose(index, false);
        return;
    }

    if (!sourceModel) {
        if (type == QStringLiteral("folder") && action == DOCK_ACTION_OPEN_IN_FILEMANAGER) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(id));
        }
        return;
    }

    if (sourceModel == m_activeAppModel || sourceModel == m_appsModel) {
        QProcess process;
        process.setProcessChannelMode(QProcess::MergedChannels);
        process.start("dde-am", {"--by-user", id, action});
        process.waitForFinished();
    }
}

void DockGlobalElementModel::requestOpenUrls(const QModelIndex &index, const QList<QUrl> &urls) const
{
    auto data = m_data.value(index.row());
    auto id = std::get<1>(data);
    auto sourceModel = std::get<2>(data);

    if (!sourceModel) {
        return;
    }

    QStringList urlStrings;
    for (const QUrl &url : urls) {
        urlStrings.append(url.toLocalFile());
    }

    QString dbusPath = QStringLiteral("/org/desktopspec/ApplicationManager1/") + escapeToObjectPath(id);
    using Application = org::desktopspec::ApplicationManager1::Application;
    Application appInterface(QStringLiteral("org.desktopspec.ApplicationManager1"), dbusPath, QDBusConnection::sessionBus());

    if (appInterface.isValid()) {
        appInterface.Launch(QString(), urlStrings, QVariantMap());
    }
}

void DockGlobalElementModel::requestClose(const QModelIndex &index, bool force) const
{
    auto data = m_data.value(index.row());
    auto sourceModel = std::get<2>(data);
    auto sourceRow = std::get<3>(data);

    if (sourceModel == m_activeAppModel) {
        auto sourceIndex = sourceModel->index(sourceRow, 0);
        m_activeAppModel->requestClose(sourceIndex, force);
        return;
    }

    qWarning() << "unable to close app not running";
}

void DockGlobalElementModel::requestUpdateWindowIconGeometry(const QModelIndex &index, const QRect &geometry, QObject *delegate) const
{
    auto data = m_data.value(index.row());
    auto sourceModel = std::get<2>(data);
    auto sourceRow = std::get<3>(data);

    if (sourceModel == m_activeAppModel) {
        auto sourceIndex = sourceModel->index(sourceRow, 0);
        m_activeAppModel->requestUpdateWindowIconGeometry(sourceIndex, geometry, delegate);
    }
}

void DockGlobalElementModel::requestWindowsView(const QModelIndexList &indexes) const
{
    Q_UNUSED(indexes)
}

void DockGlobalElementModel::moveItem(int from, int to)
{
    if (from < 0 || from >= m_data.size() || to < 0 || to >= m_data.size() || from == to)
        return;

    int destRow = from < to ? to + 1 : to;

    if (!beginMoveRows(QModelIndex(), from, from, QModelIndex(), destRow))
        return;

    m_data.move(from, to);
    endMoveRows();
}

void DockGlobalElementModel::groupItemsByApp()
{
    if (m_data.isEmpty())
        return;

    if (TaskManagerSettings::instance()->isWindowSplit())
        return;

    for (int i = 0; i < m_data.size(); ++i) {
        const QString currentType = std::get<0>(m_data.at(i));
        const QString currentId = std::get<1>(m_data.at(i));

        if (currentType != QStringLiteral("desktop")) {
            continue;
        }

        int insertPos = i + 1;

        while (insertPos < m_data.size() &&
               std::get<0>(m_data.at(insertPos)) == QStringLiteral("desktop") &&
               std::get<1>(m_data.at(insertPos)) == currentId) {
            ++insertPos;
        }

        for (int j = insertPos; j < m_data.size(); ++j) {
            if (std::get<0>(m_data.at(j)) != QStringLiteral("desktop") || std::get<1>(m_data.at(j)) != currentId)
                continue;

            int destRow = insertPos < j ? insertPos : insertPos + 1;
            if (!beginMoveRows(QModelIndex(), j, j, QModelIndex(), destRow))
                continue;
            m_data.move(j, insertPos);
            endMoveRows();

            ++insertPos;
        }

        i = insertPos - 1;
    }
}
}
