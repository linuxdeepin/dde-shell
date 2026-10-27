// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dockglobalelementmodel.h"
#include "applicationinterface.h"
#include "foldercontent.h"
#include "globals.h"
#include "taskmanager.h"
#include "taskmanagersettings.h"

#include <QAbstractListModel>
#include <QCoreApplication>
#include <QDBusConnection>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QProcess>

Q_LOGGING_CATEGORY(dockGlobalElementModelLog, "org.deepin.dde.shell.dock.taskmanager.dockglobalelementmodel")

#include <algorithm>
#include <tuple>

namespace dock
{
DockGlobalElementModel::DockGlobalElementModel(QAbstractItemModel *appsModel, DockCombineModel *activeAppModel, QObject *parent)
    : QAbstractListModel(parent)
    , AbstractTaskManagerInterface(nullptr)
    , m_appsModel(appsModel)
    , m_activeAppModel(activeAppModel)
{
    connect(TaskManagerSettings::instance(), &TaskManagerSettings::dockedElementsChanged, this, &DockGlobalElementModel::loadDockedElements);
    connect(TaskManagerSettings::instance(), &TaskManagerSettings::windowSplitChanged, this, &DockGlobalElementModel::groupItemsByApp);
    connect(TaskManagerSettings::instance(), &TaskManagerSettings::dockedApplicationsEnabledChanged, this, &DockGlobalElementModel::loadDockedElements);

    connect(
        m_appsModel,
        &QAbstractItemModel::rowsRemoved,
        this,
        [this](const QModelIndex &parent, int first, int last) {
            Q_UNUSED(parent)
            for (int i = first; i <= last; ++i) {
                auto it = std::find_if(m_data.begin(), m_data.end(), [this, &i](auto data) {
                    return std::get<1>(data) == m_appsModel && std::get<2>(data) == i;
                });
                if (it != m_data.end()) {
                    auto pos = it - m_data.begin();
                    beginRemoveRows(QModelIndex(), pos, pos);
                    m_data.remove(pos);
                    endRemoveRows();
                }
            }
            std::for_each(m_data.begin(), m_data.end(), [this, first, last](auto &data) {
                if (std::get<1>(data) == m_appsModel && std::get<2>(data) >= first) {
                    data = std::make_tuple(std::get<0>(data), std::get<1>(data), std::get<2>(data) - ((last - first) + 1));
                }
            });
        },
        Qt::QueuedConnection);

    // Keep the cached apps-model sourceRow in sync when apps are inserted.
    // Without this the cached sourceRow drifts during the startup app-model
    // rebuild and docked items resolve to the wrong app (wrong icon/name).
    connect(
        m_appsModel,
        &QAbstractItemModel::rowsInserted,
        this,
        [this](const QModelIndex &parent, int first, int last) {
            Q_UNUSED(parent)
            const int insertedCount = (last - first) + 1;
            std::for_each(m_data.begin(), m_data.end(), [this, first, insertedCount](auto &data) {
                if (std::get<1>(data) == m_appsModel && std::get<2>(data) >= first) {
                    data = std::make_tuple(std::get<0>(data), std::get<1>(data), std::get<2>(data) + insertedCount);
                }
            });

            for (auto &data : m_data) {
                if (std::get<1>(data) != m_appsModel || std::get<2>(data) != -1)
                    continue;
                const auto id = std::get<0>(data);
                auto res = m_appsModel->match(m_appsModel->index(0, 0), TaskManager::DesktopIdRole, id, 1, Qt::MatchExactly);
                std::get<2>(data) = res.isEmpty() ? -1 : res.first().row();
            }
        },
        Qt::QueuedConnection);

    // Apps model full rebuild (modelReset): re-resolve every cached sourceRow.
    connect(
        m_appsModel,
        &QAbstractItemModel::modelReset,
        this,
        [this]() {
            for (auto &data : m_data) {
                if (std::get<1>(data) != m_appsModel)
                    continue;
                const auto id = std::get<0>(data);
                auto res = m_appsModel->match(m_appsModel->index(0, 0), TaskManager::DesktopIdRole, id, 1, Qt::MatchExactly);
                std::get<2>(data) = res.isEmpty() ? -1 : res.first().row();
            }
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
                //将同一应用的窗口添加到一起 
                // Find the first occurrence of this app in m_data (either docked item or existing window)
                auto firstIt = std::find_if(m_data.begin(), m_data.end(), [&desktopId](const auto &data) {
                    return std::get<0>(data) == desktopId;
                });

                if (firstIt == m_data.end()) {
                    // No docked item or existing window yet, append to the end
                    beginInsertRows(QModelIndex(), m_data.size(), m_data.size());
                    m_data.append(std::make_tuple(desktopId, m_activeAppModel, i));
                    endInsertRows();
                    continue;
                }

                // If the first occurrence still comes from m_appsModel, this is the first window:
                // reuse the docked position and turn it into a running window.
                if (std::get<1>(*firstIt) == m_appsModel) {
                    *firstIt = std::make_tuple(desktopId, m_activeAppModel, i);
                    auto pIndex = this->index(firstIt - m_data.begin(), 0);
                    Q_EMIT dataChanged(pIndex,
                                       pIndex,
                                       {TaskManager::ActiveRole,
                                        TaskManager::AttentionRole,
                                        TaskManager::WindowsRole,
                                        TaskManager::MenusRole,
                                        TaskManager::WinTitleRole,
                                        TaskManager::IconNameRole,
                                        TaskManager::NameRole});
                    continue;
                }

                // There are already windows for this app: insert the new window
                // right after the last (rightmost) existing one.
                // Search the entire list since windows may not be consecutive after drag reorder.
                auto lastIt = firstIt;
                for (auto it = firstIt + 1; it != m_data.end(); ++it) {
                    if (std::get<0>(*it) == desktopId) {
                        lastIt = it;
                    }
                }

                auto insertRow = (lastIt - m_data.begin()) + 1;
                beginInsertRows(QModelIndex(), insertRow, insertRow);
                m_data.insert(lastIt + 1, std::make_tuple(desktopId, m_activeAppModel, i));
                endInsertRows();
            }

            std::for_each(m_data.begin(), m_data.end(), [this, first, last](auto &data) {
                if (std::get<1>(data) == m_activeAppModel && std::get<2>(data) > first) {
                    data = std::make_tuple(std::get<0>(data),
                                           std::get<1>(data),
                                           std::get<2>(data) + ((last - first) + 1));
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
            // apps that are still running and whose cached active-model row must be
            // refreshed once the remaining rows have been renumbered
            QStringList pendingRowRefresh;

            for (int i = first; i <= last; ++i) {
                auto it = std::find_if(m_data.begin(), m_data.end(), [this, i](auto data) {
                    return std::get<1>(data) == m_activeAppModel && std::get<2>(data) == i;
                });

                if (it == m_data.end()) {
                    qWarning() << "failed to find a running apps on dock" << i;
                    continue;
                }

                auto pos = it - m_data.begin();
                auto id = std::get<0>(*it);

                auto oit = std::find_if(m_data.constBegin(), m_data.constEnd(), [this, &id, i](auto &data) {
                    return std::get<0>(data) == id && std::get<1>(data) == m_activeAppModel && std::get<2>(data) != i;
                });

                if (oit == m_data.constEnd()) {
                    // Sole entry for this app. The notification above is queued, so the
                    // active-app model may already represent another window of the same
                    // app by now: an app keeps one row while its window is replaced,
                    // which is what happens when an Electron splash window closes and
                    // the main window appears. Only "is this app still running" matters,
                    // not which row was reported as removed.
                    if (findActiveRowByDesktopId(id) >= 0) {
                        pendingRowRefresh.append(id);
                        continue;
                    }

                    if (m_dockedElements.contains(std::make_tuple("desktop", id))) {
                        auto res = m_appsModel->match(m_appsModel->index(0, 0), TaskManager::DesktopIdRole, id, 1, Qt::MatchExactly);
                        if (res.isEmpty()) {
                            beginRemoveRows(QModelIndex(), pos, pos);
                            m_data.remove(pos);
                            endRemoveRows();
                        } else {
                            *it = std::make_tuple(id, m_appsModel, res.first().row());
                            // DEFER emitter until internal model shift is done!
                            pendingDataChangedRows.append(pos);
                        }
                    } else {
                        beginRemoveRows(QModelIndex(), pos, pos);
                        m_data.remove(pos);
                        endRemoveRows();
                    }
                    continue;
                }

                // The app still has other windows on the dock; drop only this one.
                beginRemoveRows(QModelIndex(), pos, pos);
                m_data.remove(pos);
                endRemoveRows();
            }

            // Adjust remaining row mappings for the active app model BEFORE any outer access
            std::for_each(m_data.begin(), m_data.end(), [this, first, last](auto &data) {
                if (std::get<1>(data) == m_activeAppModel && std::get<2>(data) >= first) {
                    data = std::make_tuple(std::get<0>(data), std::get<1>(data), std::get<2>(data) - ((last - first) + 1));
                }
            });

            // Re-point entries of apps that are still running at their current row.
            // Done after the renumbering above, which would otherwise invalidate them.
            for (const auto &id : std::as_const(pendingRowRefresh)) {
                const int liveRow = findActiveRowByDesktopId(id);
                if (liveRow < 0)
                    continue;
                auto refreshIt = std::find_if(m_data.begin(), m_data.end(), [&id](const auto &data) {
                    return std::get<0>(data) == id;
                });
                if (refreshIt == m_data.end())
                    continue;
                *refreshIt = std::make_tuple(id, m_activeAppModel, liveRow);
                pendingDataChangedRows.append(refreshIt - m_data.begin());
            }

            // Now it is safe to emit dataChanged for rows that were swapped to docked elements
            for (int pos : pendingDataChangedRows) {
                if (pos >= m_data.size())
                    continue;
                auto pIndex = this->index(pos, 0);
                Q_EMIT dataChanged(pIndex,
                                   pIndex,
                                   {TaskManager::ActiveRole, TaskManager::AttentionRole, TaskManager::WindowsRole, TaskManager::MenusRole, TaskManager::WinTitleRole, TaskManager::IconNameRole, TaskManager::NameRole});
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
                    return std::get<1>(data) == m_activeAppModel && std::get<2>(data) == i;
                });

                if (it == m_data.end())
                    continue;
                auto pos = it - m_data.constBegin();

                auto oldRoles = roles;
                auto desktopId = roles.indexOf(TaskManager::DesktopIdRole);
                auto identifyId = roles.indexOf(TaskManager::IdentityRole);
                if (desktopId != -1 || identifyId != -1) {
                    oldRoles.append(TaskManager::ItemIdRole);
                }
                Q_EMIT dataChanged(index(pos, 0), index(pos, 0), oldRoles);
            }
        },
        Qt::QueuedConnection);

    connect(
        m_appsModel,
        &QAbstractItemModel::dataChanged,
        this,
        [this](const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles) {
            int first = topLeft.row(), last = bottomRight.row();
            for (int i = first; i <= last; i++) {
                auto id = m_appsModel->index(i, 0).data(TaskManager::DesktopIdRole).toString();
                if (id.isEmpty())
                    continue;

                auto it = std::find_if(m_data.begin(), m_data.end(), [&id](const auto &data) {
                    return std::get<0>(data) == id;
                });

                if (it == m_data.end())
                    continue;

                auto pos = it - m_data.begin();
                Q_EMIT dataChanged(index(pos, 0), index(pos, 0), roles);
            }
        },
        Qt::QueuedConnection);

    QMetaObject::invokeMethod(this, &DockGlobalElementModel::loadDockedElements, Qt::QueuedConnection);
}

QHash<int, QByteArray> DockGlobalElementModel::roleNames() const
{
    return {
        {TaskManager::ApplicationFolderContentsRole, MODEL_APPLICATIONFOLDERCONTENTS},
        {TaskManager::FolderTypeRole, MODEL_FOLDERTYPE},
        {TaskManager::FolderPresentationRole, MODEL_FOLDERPRESENTATION},
        {TaskManager::ItemIdRole, MODEL_ITEMID},
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

void DockGlobalElementModel::initDockedElements(bool unused)
{
    Q_UNUSED(unused);
    loadDockedElements();
}

void DockGlobalElementModel::loadDockedElements()
{
    QList<std::tuple<QString, QString>> newDocked;
    if (TaskManagerSettings::instance()->dockedApplicationsEnabled()) {
        const auto configured = TaskManagerSettings::instance()->dockedElements();
        for (int configuredIndex = 0; configuredIndex < configured.size(); ++configuredIndex) {
            const auto &elementInfo = configured.at(configuredIndex);
            auto pair = elementInfo.split('/');
            const bool content = elementInfo.startsWith("folder/");
            if (content && FolderContentRegistry::instance()->normalizeReference(elementInfo) != elementInfo)
                continue;
            const bool folder = content || elementInfo.startsWith("internal/folders/");
            if (!folder && (pair.size() != 2 || pair[0] != "desktop")) continue;
            if (folder && !content && !containsApplicationFolder(elementInfo)) continue;
            auto type = folder ? QStringLiteral("group") : pair[0];
            auto id = folder ? elementInfo : pair[1];

            if (content)
                ensureFolderPresentation(id);
            auto tmp = std::make_tuple(type, id);

            // check desktop is installed
            QAbstractItemModel *model = nullptr;
            int row = 0;
            if (type == "desktop") {
                model = m_appsModel;
                auto res = m_appsModel->match(m_appsModel->index(0, 0), TaskManager::DesktopIdRole, id, 1, Qt::MatchExactly).value(0);
                if (!res.isValid())
                    continue;
                row = res.row();
            }

            if (newDocked.contains(tmp)) continue;
            newDocked.append(tmp);

            auto isRunning = std::any_of(m_data.constBegin(), m_data.constEnd(), [&id](const auto &data) {
                return std::get<0>(data) == id;
            });

            if (!isRunning) {
                // A folder can arrive before the applications provider is ready.
                // Insert newly available pins relative to their configured peers,
                // rather than appending and losing the saved mixed folder/app order.
                int position = m_data.size();
                for (int next = configuredIndex + 1; next < configured.size(); ++next) {
                    const auto nextId = configured[next].startsWith("desktop/") ? configured[next].mid(8) : configured[next];
                    auto following = std::find_if(m_data.cbegin(), m_data.cend(), [&nextId](const auto &entry) {
                        return std::get<0>(entry) == nextId;
                    });
                    if (following != m_data.cend()) {
                        position = following - m_data.cbegin();
                        break;
                    }
                }
                beginInsertRows(QModelIndex(), position, position);
                m_data.insert(position, std::make_tuple(id, model, row));
                endInsertRows();
            }
        }
    }

    for (auto it = m_dockedElements.begin(); it < m_dockedElements.end(); ++it) {
        if (newDocked.contains(*it))
            continue;
        const auto &id = std::get<1>(*it);
        auto dataIt = std::find_if(m_data.begin(), m_data.end(), [this, &id](const auto &data) {
            return std::get<0>(data) == id && (std::get<1>(data) == m_appsModel || std::get<1>(data) == nullptr);
        });
        if (dataIt != m_data.end()) {
            auto pos = (dataIt - m_data.begin());
            beginRemoveRows(QModelIndex(), pos, pos);
            m_data.remove(pos);
            endRemoveRows();
        }
    }

    m_dockedElements = newDocked;

    for (const auto &id : m_folderPresentations.keys()) {
        if (!newDocked.contains(std::make_tuple(QStringLiteral("group"), id))) {
            if (auto presentation = m_folderPresentations.take(id)) presentation->deleteLater();
        }
    }

    qCDebug(dockGlobalElementModelLog) << "loaded docked elements count:" << m_dockedElements.count() << "appsModel row count:" << m_appsModel->rowCount();

    if (!m_data.isEmpty()) {
        // MenusRole should also be handled here due to it contains the copywriting of docked or undocked
        Q_EMIT dataChanged(index(0, 0), index(m_data.size() - 1, 0), {TaskManager::DockedRole, TaskManager::MenusRole, TaskManager::IconNameRole, TaskManager::NameRole, TaskManager::ApplicationFolderContentsRole});
    }
}

void DockGlobalElementModel::ensureFolderPresentation(const QString &id)
{
    if (m_folderPresentations.contains(id))
        return;

    auto presentation = FolderContentRegistry::instance()->create(id, this);
    m_folderPresentations.insert(id, presentation);
    if (!presentation)
        return;

    // Only refresh presentations still registered; removed objects may be
    // waiting for deferred deletion after an undock or rename.
    connect(presentation, &FolderPresentation::presentationChanged, this, [this, presentation] {
        const auto id = m_folderPresentations.key(presentation);
        if (id.isEmpty()) return;
        for (int i = 0; i < m_data.size(); ++i)
            if (std::get<0>(m_data.at(i)) == id)
                emit dataChanged(index(i, 0), index(i, 0), {TaskManager::NameRole});
    });
    connect(presentation, &FolderPresentation::identityChanged, this,
            [this, presentation](const QString &newId) {
                rebindFolderElement(presentation, newId);
            });
    connect(presentation, &FolderPresentation::contentLost, this,
            [this, presentation] {
                dropFolderElement(presentation);
            });
}

QString DockGlobalElementModel::getMenus(const QModelIndex &index) const
{
    auto data = m_data.at(index.row());
    auto id = std::get<0>(data);
    auto model = std::get<1>(data);
    auto row = std::get<2>(data);

    QJsonArray menusArray;
    QString appNameInMenu = tr("Open");
    if (model == m_activeAppModel) {
        appNameInMenu = index.data(TaskManager::NameRole).toString();
        // In case a window does not belongs to a known application, use the window title instead
        if (appNameInMenu.isEmpty()) {
            appNameInMenu = index.data(TaskManager::WinTitleRole).toString();
        }
    }
    menusArray.append(QJsonObject{{"id", ""}, {"name", appNameInMenu}});

    auto actions = model->index(row, 0).data(TaskManager::ActionsRole).toByteArray();
    for (auto action : QJsonDocument::fromJson(actions).array()) {
        menusArray.append(action);
    }

    if (TaskManagerSettings::instance()->dockedApplicationsEnabled()) {
        bool isDocked = (model == nullptr) || m_dockedElements.contains(std::make_tuple("desktop", id));
        menusArray.append(QJsonObject{{"id", DOCK_ACTION_DOCK}, {"name", isDocked ? tr("Undock") : tr("Dock")}});
    }

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
    if (!index.isValid() || index.row() >= m_data.size())
        return {};

    auto data = m_data.at(index.row());
    auto id = std::get<0>(data);
    auto model = std::get<1>(data);
    auto row = std::get<2>(data);

    QString folderType;
    if (!model) {
        // References are validated before insertion by loadDockedElements().
        if (id.startsWith("folder/"))
            folderType = id.section('/', 1, 1);
        else if (id.startsWith("internal/folders/"))
            folderType = QStringLiteral("launcher");
    }
    if (!folderType.isEmpty()) {
        const auto presentation = m_folderPresentations.value(id);
        switch (role) {
        case TaskManager::ItemIdRole:
        case TaskManager::DesktopIdRole: return id;
        case TaskManager::NameRole:
            if (folderType != "launcher")
                return presentation ? presentation->title() : tr("Folder");
            return applicationFolderName(id);
        case TaskManager::IconNameRole: return QStringLiteral("folder");
        case TaskManager::WinTitleRole:
        case TaskManager::WinIconRole: return QString();
        case TaskManager::FolderTypeRole: return folderType;
        case TaskManager::FolderPresentationRole: return QVariant::fromValue(static_cast<QObject *>(presentation));
        case TaskManager::ApplicationFolderContentsRole:
            if (folderType != "launcher")
                return QVariantList();
            return applicationFolderContents(id);
        case TaskManager::WindowsRole: return QStringList();
        case TaskManager::ActiveRole:
        case TaskManager::AttentionRole: return false;
        case TaskManager::DockedRole: return true;
        // FolderContextMenu obtains both standard and plugin actions on open.
        case TaskManager::MenusRole: return QStringLiteral("[]");
        default: return {};
        }
    }
    if (role == TaskManager::FolderTypeRole) return QString();
    if (role == TaskManager::ApplicationFolderContentsRole) return QVariantList();
    if (role == TaskManager::FolderPresentationRole) return QVariant::fromValue(static_cast<QObject *>(nullptr));

    // cached sourceRow can be out of range after the apps model is rebuilt;
    // re-resolve by desktopId instead of reading a wrong row
    if (model == m_appsModel && (row < 0 || row >= model->rowCount())) {
        auto res = m_appsModel->match(m_appsModel->index(0, 0), TaskManager::DesktopIdRole, id, 1, Qt::MatchExactly);
        row = res.isEmpty() ? -1 : res.first().row();
    }

    switch (role) {
    case TaskManager::ItemIdRole:
        return id;
    case TaskManager::WindowsRole: {
        if (model == m_activeAppModel) {
            if (row < 0 || row >= model->rowCount())
                return {};
            return QStringList{model->index(row, 0).data(TaskManager::WinIdRole).toString()};
        }
        // For m_appsModel data, when it's GroupModel we can directly get all window IDs for this desktop ID
        if (row < 0 || row >= model->rowCount())
            return {};
        QModelIndex groupIndex = model->index(row, 0);
        return groupIndex.data(TaskManager::WindowsRole).toStringList();
    }
    case TaskManager::ActiveRole:
    case TaskManager::AttentionRole: {
        if (model == m_activeAppModel) {
            if (row < 0 || row >= model->rowCount())
                return false;
            return model->index(row, 0).data(role);
        }
        return false;
    }

    case TaskManager::MenusRole: {
        return getMenus(index);
    }

    default: {
        if (model) {
            if (row < 0 || row >= model->rowCount())
                return {};
            return model->index(row, 0).data(role);
        }
        return {};
    }
    }
}

void DockGlobalElementModel::requestActivate(const QModelIndex &index) const
{
    auto data = m_data.value(index.row());
    auto sourceModel = std::get<1>(data);
    auto sourceRow = std::get<2>(data);

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
    if (!index.isValid() || index.row() >= m_data.size()) return;
    auto data = m_data.value(index.row());
    if (!std::get<1>(data)) return;
    auto id = std::get<0>(data);
    qDebug(dockGlobalElementModelLog) << "Requesting new instance for index:" << index << "with action:" << action << "id:" << id;

    // Handle special actions first (for both active and docked apps)
    if (action == DOCK_ACTION_DOCK) {
        TaskManagerSettings::instance()->toggleDockedElement(QStringLiteral("desktop/%1").arg(id));
        return;
    } else if (action == DOCK_ACTION_FORCEQUIT) {
        requestClose(index, true);
        return;
    } else if (action == DOCK_ACTION_CLOSEWINDOW || action == DOCK_ACTION_CLOSEALL) {
        requestClose(index, false);
        return;
    } else {
        QProcess process;
        process.setProcessChannelMode(QProcess::MergedChannels);
#ifdef HAVE_DDE_API_EVENTLOGGER
        process.start("dde-am", {"--by-user", "--launch-type", "dde-shell", id, action});
#else
        process.start("dde-am", {"--by-user", id, action});
#endif
        process.waitForFinished();
    }
}

void DockGlobalElementModel::requestOpenUrls(const QModelIndex &index, const QList<QUrl> &urls) const
{
    if (!index.isValid() || index.row() >= m_data.size()) return;
    auto data = m_data.value(index.row());
    if (!std::get<1>(data)) return;
    auto id = std::get<0>(data);

    QStringList urlStrings;
    for (const QUrl &url : urls) {
        urlStrings.append(url.toLocalFile());
    }

    QString dbusPath = QStringLiteral("/org/desktopspec/ApplicationManager1/") + escapeToObjectPath(id);
    using Application = org::desktopspec::ApplicationManager1::Application;
    Application appInterface(QStringLiteral("org.desktopspec.ApplicationManager1"), dbusPath, QDBusConnection::sessionBus());

    if (appInterface.isValid()) {
        appInterface.Launch(QString(), urlStrings, QVariantMap{{QStringLiteral("_launch_type"), QStringLiteral("dde-shell")}});
    }
}

void DockGlobalElementModel::requestClose(const QModelIndex &index, bool force) const
{
    if (!index.isValid() || index.row() >= m_data.size()) return;
    auto data = m_data.value(index.row());
    if (!std::get<1>(data)) return;
    auto sourceModel = std::get<1>(data);
    auto sourceRow = std::get<2>(data);

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
    auto sourceModel = std::get<1>(data);
    auto sourceRow = std::get<2>(data);

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

int DockGlobalElementModel::findActiveRowByDesktopId(const QString &id) const
{
    if (!m_activeAppModel || id.isEmpty())
        return -1;

    const auto rows = m_activeAppModel->match(m_activeAppModel->index(0, 0),
                                              TaskManager::DesktopIdRole, id, 1,
                                              Qt::MatchExactly);
    return rows.isEmpty() ? -1 : rows.first().row();
}

void DockGlobalElementModel::groupItemsByApp()
{
    if (m_data.isEmpty())
        return;

    if (TaskManagerSettings::instance()->isWindowSplit())
        return;

    for (int i = 0; i < m_data.size(); ++i) {
        const QString currentId = std::get<0>(m_data.at(i));

        int insertPos = i + 1;

        while (insertPos < m_data.size() && std::get<0>(m_data.at(insertPos)) == currentId) {
            ++insertPos;
        }

        for (int j = insertPos; j < m_data.size(); ++j) {
            if (std::get<0>(m_data.at(j)) != currentId)
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
void DockGlobalElementModel::setApplicationFolderModel(QAbstractItemModel *model)
{
    if (m_applicationFolderModel) disconnect(m_applicationFolderModel, nullptr, this, nullptr);
    m_applicationFolderModel = model;
    if (model) {
        // One notification after the complete operation also covers row changes.
        connect(model, SIGNAL(arrangementChanged()), this, SLOT(syncApplicationFolders()));
    }
    syncApplicationFolders();
}

QString DockGlobalElementModel::applicationFolderName(const QString &id) const
{
    const auto folder = applicationFolderIndex(id);
    if (!folder.isValid()) return {};
    const int role = m_applicationFolderModel->roleNames().key(MODEL_NAME, -1);
    const QString name = role >= 0 ? folder.data(role).toString() : QString();
    if (name.startsWith("internal/category/")) {
        static const char *names[] = {"Internet", "Chat", "Music", "Video", "Graphics", "Games", "Office", "Reading", "Development", "System", "Others"};
        bool ok = false;
        const int category = name.mid(18).toInt(&ok);
        return QCoreApplication::translate("Main", names[ok && category >= 0 && category < 11 ? category : 10]);
    }
    return name;
}

QVariantList DockGlobalElementModel::applicationFolderContents(const QString &id) const
{
    QVariantMap info;
    if (m_applicationFolderModel) QMetaObject::invokeMethod(m_applicationFolderModel, "folderInfo", Q_RETURN_ARG(QVariantMap, info), Q_ARG(QString, id));
    return info.value("contents").toList();
}

void DockGlobalElementModel::rebindFolderElement(FolderPresentation *presentation, const QString &newId)
{
    const auto oldId = m_folderPresentations.key(presentation);
    if (oldId.isEmpty()) return;

    const auto registry = FolderContentRegistry::instance();
    const QString newReference = registry->referenceWithId(oldId, newId);
    if (newReference.isEmpty()) return;
    // The provider already reports the canonical form; a no-op means the item
    // did not actually move and nothing has to be persisted.
    if (newReference == oldId) return;

    auto settings = TaskManagerSettings::instance();
    auto elements = settings->dockedElements();
    const int position = elements.indexOf(oldId);
    if (position < 0) return;
    elements.replace(position, newReference);
    settings->setDockedElements(elements);   // reloads this model with the new id

    qCDebug(dockGlobalElementModelLog) << "folder pin rebound after rename:" << newReference;
}

void DockGlobalElementModel::dropFolderElement(FolderPresentation *presentation)
{
    const auto lostId = m_folderPresentations.key(presentation);
    if (lostId.isEmpty()) return;

    auto settings = TaskManagerSettings::instance();
    auto elements = settings->dockedElements();
    if (elements.removeAll(lostId) == 0) return;
    settings->setDockedElements(elements);   // reloads this model without the pin

    qCDebug(dockGlobalElementModelLog) << "folder pin dropped, directory is gone:" << lostId;
}

QModelIndex DockGlobalElementModel::applicationFolderIndex(const QString &id) const
{
    if (!m_applicationFolderModel || !id.startsWith("internal/folders/") || id == "internal/folders/0") return {};
    // Check identity independently of application metadata availability. Exact
    // matching also rejects aliases and malformed external drag data.
    const int role = m_applicationFolderModel->roleNames().key(MODEL_DESKTOPID, -1);
    return role >= 0 ? m_applicationFolderModel->match(m_applicationFolderModel->index(0, 0), role, id, 1, Qt::MatchExactly).value(0) : QModelIndex();
}

bool DockGlobalElementModel::containsApplicationFolder(const QString &id) const
{
    return applicationFolderIndex(id).isValid();
}

void DockGlobalElementModel::syncApplicationFolders()
{
    if (!m_applicationFolderModel) return; // An unavailable provider is not a deleted folder.
    auto settings = TaskManagerSettings::instance();
    auto elements = settings->dockedElements();
    const auto removed = elements.removeIf([this](const QString &id) {
        return id.startsWith("internal/folders/") && !containsApplicationFolder(id);
    });
    if (removed) settings->setDockedElements(elements);
    else loadDockedElements();
}

}
