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
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QProcess>

Q_LOGGING_CATEGORY(dockGlobalElementModelLog, "org.deepin.dde.shell.dock.taskmanager.dockglobalelementmodel")

#include <algorithm>
#include <tuple>

namespace dock
{
namespace {

bool isGroupElement(const QString &id, const QAbstractItemModel *model)
{
    return model == nullptr && id.startsWith(QStringLiteral("internal/folders/"));
}

int modelRole(const QAbstractItemModel *model, const QByteArray &name, int fallback = -1)
{
    return model ? model->roleNames().key(name, fallback) : fallback;
}

}

DockGlobalElementModel::DockGlobalElementModel(QAbstractItemModel *appsModel, QAbstractItemModel *groupModel, QObject *groupProvider,
                                               DockCombineModel *activeAppModel, QObject *parent)
    : QAbstractListModel(parent)
    , AbstractTaskManagerInterface(nullptr)
    , m_appsModel(appsModel)
    , m_groupModel(groupModel)
    , m_groupProvider(groupProvider)
    , m_activeAppModel(activeAppModel)
{
    connect(TaskManagerSettings::instance(), &TaskManagerSettings::dockedElementsChanged, this, &DockGlobalElementModel::loadDockedElements);
    connect(TaskManagerSettings::instance(), &TaskManagerSettings::windowSplitChanged, this, &DockGlobalElementModel::groupItemsByApp);
    connect(TaskManagerSettings::instance(), &TaskManagerSettings::dockedApplicationsEnabledChanged, this, &DockGlobalElementModel::loadDockedElements);

    if (m_groupModel) {
        connect(m_groupModel, &QAbstractItemModel::modelReset, this, &DockGlobalElementModel::refreshGroupElements, Qt::QueuedConnection);
        connect(m_groupModel, &QAbstractItemModel::rowsInserted, this, &DockGlobalElementModel::refreshGroupElements, Qt::QueuedConnection);
        connect(m_groupModel, &QAbstractItemModel::rowsRemoved, this, &DockGlobalElementModel::handleGroupRowsRemoved, Qt::QueuedConnection);
        connect(m_groupModel, &QAbstractItemModel::dataChanged, this, &DockGlobalElementModel::notifyGroupElementsChanged, Qt::QueuedConnection);
    }

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
                                        TaskManager::WinTitleRole});
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

                if (oit == m_data.constEnd() && m_dockedElements.contains(std::make_tuple("desktop", id))) {
                    auto res = m_appsModel->match(m_appsModel->index(0, 0), TaskManager::DesktopIdRole, id, 1, Qt::MatchExactly);
                    if (res.isEmpty()) {
                        beginRemoveRows(QModelIndex(), pos, pos);
                        m_data.remove(pos);
                        endRemoveRows();
                    } else {
                        auto row = res.first().row();
                        *it = std::make_tuple(id, m_appsModel, row);
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
                if (std::get<1>(data) == m_activeAppModel && std::get<2>(data) >= first) {
                    data = std::make_tuple(std::get<0>(data), std::get<1>(data), std::get<2>(data) - ((last - first) + 1));
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
                    return std::get<1>(data) == m_activeAppModel && std::get<2>(data) == i;
                });

                if (it == m_data.end())
                    return;
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

    QMetaObject::invokeMethod(this, &DockGlobalElementModel::loadDockedElements, Qt::QueuedConnection);
}

QHash<int, QByteArray> DockGlobalElementModel::roleNames() const
{
    return {
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
        {TaskManager::GroupItemsRole, MODEL_GROUPITEMS},
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
        for (auto elementInfo : TaskManagerSettings::instance()->dockedElements()) {
            QString type;
            QString id;
            if (elementInfo.startsWith(QStringLiteral("internal/folders/"))) {
                type = QStringLiteral("group");
                id = elementInfo;
            } else {
                const auto separator = elementInfo.indexOf('/');
                if (separator <= 0 || separator == elementInfo.size() - 1)
                    continue;

                type = elementInfo.left(separator);
                id = elementInfo.mid(separator + 1);
                if (type != QStringLiteral("desktop"))
                    continue;
            }

            auto tmp = std::make_tuple(type, id);

            // check desktop is installed
            QAbstractItemModel *model = nullptr;
            int row = 0;
            if (type == QStringLiteral("desktop")) {
                model = m_appsModel;
                auto res = m_appsModel->match(m_appsModel->index(0, 0), TaskManager::DesktopIdRole, id, 1, Qt::MatchExactly).value(0);
                if (!res.isValid())
                    continue;
                row = res.row();
            } else if (!groupIndex(id).isValid()) {
                continue;
            }

            newDocked.append(tmp);
            if (m_dockedElements.contains(tmp))
                continue;

            auto isRunning = std::any_of(m_data.constBegin(), m_data.constEnd(), [&type, &id](const auto &data) {
                if (std::get<0>(data) != id)
                    return false;
                return isGroupElement(std::get<0>(data), std::get<1>(data)) == (type == QStringLiteral("group"));
            });

            if (!isRunning) {
                beginInsertRows(QModelIndex(), m_data.size(), m_data.size());
                m_data.append(std::make_tuple(id, model, row));
                endInsertRows();
            }
        }
    }

    for (auto it = m_dockedElements.begin(); it < m_dockedElements.end(); ++it) {
        if (newDocked.contains(*it))
            continue;
        auto type = std::get<0>(*it), id = std::get<1>(*it);
        auto dataIt = std::find_if(m_data.begin(), m_data.end(), [this, &type, &id](const auto &item) {
            if (std::get<0>(item) != id)
                return false;
            if (type == QStringLiteral("group"))
                return isGroupElement(std::get<0>(item), std::get<1>(item));
            return std::get<1>(item) == m_appsModel;
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
        Q_EMIT dataChanged(index(0, 0), index(m_data.size() - 1, 0), {TaskManager::DockedRole, TaskManager::MenusRole});
    }
}

QString DockGlobalElementModel::getMenus(const QModelIndex &index) const
{
    auto data = m_data.at(index.row());
    auto id = std::get<0>(data);
    auto model = std::get<1>(data);
    auto row = std::get<2>(data);

    if (isGroupElement(id, model)) {
        QJsonArray menusArray;
        if (TaskManagerSettings::instance()->dockedApplicationsEnabled()) {
            const bool isDocked = m_dockedElements.contains(std::make_tuple(QStringLiteral("group"), id));
            menusArray.append(QJsonObject{{"id", DOCK_ACTION_DOCK}, {"name", isDocked ? tr("Undock") : tr("Dock")}});
        }
        return QJsonDocument(menusArray).toJson();
    }

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
    if (index.row() >= m_data.size())
        return {};

    auto data = m_data.at(index.row());
    auto id = std::get<0>(data);
    auto model = std::get<1>(data);
    auto row = std::get<2>(data);

    if (isGroupElement(id, model)) {
        switch (role) {
        case TaskManager::ItemIdRole:
        case TaskManager::DesktopIdRole:
            return id;
        case TaskManager::GroupItemsRole:
            return groupApplications(id);
        case TaskManager::NameRole:
            return groupDisplayName(id);
        case TaskManager::IconNameRole:
            return QStringLiteral("folder");
        case TaskManager::WindowsRole:
            return QStringList{};
        case TaskManager::ActiveRole:
        case TaskManager::AttentionRole:
            return false;
        case TaskManager::DockedRole:
            return m_dockedElements.contains(std::make_tuple(QStringLiteral("group"), id));
        case TaskManager::MenusRole:
            return getMenus(index);
        default:
            return {};
        }
    }

    switch (role) {
    case TaskManager::ItemIdRole:
        return id;
    case TaskManager::GroupItemsRole:
        return QVariantList{};
    case TaskManager::WindowsRole: {
        if (model == m_activeAppModel) {
            return QStringList{model->index(row, 0).data(TaskManager::WinIdRole).toString()};
        }
        // For m_appsModel data, when it's GroupModel we can directly get all window IDs for this desktop ID
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

    case TaskManager::DockedRole:
        return model == nullptr || m_dockedElements.contains(std::make_tuple("desktop", id));

    case TaskManager::MenusRole: {
        return getMenus(index);
    }

    default: {
        if (model) {
            return model->index(row, 0).data(role);
        }
        return {};
    }
    }
}

void DockGlobalElementModel::requestActivate(const QModelIndex &index) const
{
    auto data = m_data.value(index.row());
    auto id = std::get<0>(data);
    auto sourceModel = std::get<1>(data);
    auto sourceRow = std::get<2>(data);

    if (isGroupElement(id, sourceModel))
        return;

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
    auto id = std::get<0>(data);
    auto model = std::get<1>(data);
    qDebug(dockGlobalElementModelLog) << "Requesting new instance for index:" << index << "with action:" << action << "id:" << id;

    if (isGroupElement(id, model)) {
        if (action == DOCK_ACTION_DOCK)
            TaskManagerSettings::instance()->toggleDockedElement(id);
        return;
    }

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
    auto data = m_data.value(index.row());
    auto id = std::get<0>(data);
    auto model = std::get<1>(data);
    if (isGroupElement(id, model))
        return;

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
    auto data = m_data.value(index.row());
    auto id = std::get<0>(data);
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
    auto id = std::get<0>(data);
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

QModelIndex DockGlobalElementModel::groupIndex(const QString &launcherFolderId) const
{
    if (!m_groupModel || m_groupModel->rowCount() == 0 || launcherFolderId == QStringLiteral("internal/folders/0"))
        return {};

    const int desktopIdRole = modelRole(m_groupModel, QByteArray(MODEL_DESKTOPID));
    if (desktopIdRole < 0)
        return {};

    return m_groupModel->match(m_groupModel->index(0, 0), desktopIdRole,
                               launcherFolderId, 1, Qt::MatchExactly).value(0);
}

QString DockGlobalElementModel::groupDisplayName(const QString &launcherFolderId) const
{
    QString displayName;
    if (m_groupProvider) {
        QMetaObject::invokeMethod(m_groupProvider, "groupDisplayName", Qt::DirectConnection,
                                  Q_RETURN_ARG(QString, displayName), Q_ARG(QString, launcherFolderId));
    }

    if (!displayName.isEmpty())
        return displayName;

    const auto sourceIndex = groupIndex(launcherFolderId);
    const int displayRole = modelRole(m_groupModel, QByteArrayLiteral("display"), Qt::DisplayRole);
    const int nameRole = modelRole(m_groupModel, QByteArray(MODEL_NAME), displayRole);
    return sourceIndex.data(nameRole).toString();
}

QVariantList DockGlobalElementModel::groupApplications(const QString &launcherFolderId) const
{
    QVariantList applications;
    if (m_groupProvider) {
        QMetaObject::invokeMethod(m_groupProvider, "groupItemDetails", Qt::DirectConnection,
                                  Q_RETURN_ARG(QVariantList, applications), Q_ARG(QString, launcherFolderId));
    }

    const int desktopIdRole = modelRole(m_appsModel, QByteArray(MODEL_DESKTOPID));
    const int nameRole = modelRole(m_appsModel, QByteArray(MODEL_NAME));
    if (desktopIdRole < 0 || nameRole < 0)
        return applications;

    for (QVariant &application : applications) {
        QVariantMap details = application.toMap();
        const QString desktopId = details.value(QStringLiteral("desktopId")).toString();
        if (desktopId.isEmpty())
            continue;

        QModelIndex appIndex = m_appsModel->match(m_appsModel->index(0, 0), desktopIdRole,
                                                  desktopId, 1, Qt::MatchExactly).value(0);
        if (!appIndex.isValid()) {
            const QString alternateId = desktopId.endsWith(QStringLiteral(".desktop"))
                ? desktopId.chopped(8)
                : desktopId + QStringLiteral(".desktop");
            appIndex = m_appsModel->match(m_appsModel->index(0, 0), desktopIdRole,
                                          alternateId, 1, Qt::MatchExactly).value(0);
        }

        if (!appIndex.isValid())
            continue;

        const QString localizedName = appIndex.data(nameRole).toString();
        if (!localizedName.isEmpty()) {
            details.insert(QStringLiteral("name"), localizedName);
            application = details;
        }
    }

    return applications;
}

void DockGlobalElementModel::refreshGroupElements()
{
    loadDockedElements();
    notifyGroupElementsChanged();
}

void DockGlobalElementModel::handleGroupRowsRemoved()
{
    auto *settings = TaskManagerSettings::instance();
    QStringList dockedElements = settings->dockedElements();
    bool dockedElementsChanged = false;

    for (auto it = dockedElements.begin(); it != dockedElements.end();) {
        if (!it->startsWith(QStringLiteral("internal/folders/"))) {
            ++it;
            continue;
        }

        if (groupIndex(*it).isValid()) {
            ++it;
            continue;
        }

        it = dockedElements.erase(it);
        dockedElementsChanged = true;
    }

    if (dockedElementsChanged) {
        settings->setDockedElements(dockedElements);
    } else {
        loadDockedElements();
    }
    notifyGroupElementsChanged();
}

void DockGlobalElementModel::notifyGroupElementsChanged()
{
    if (!m_data.isEmpty()) {
        Q_EMIT dataChanged(index(0, 0), index(m_data.size() - 1, 0),
                           {TaskManager::NameRole, TaskManager::GroupItemsRole});
    }
}

bool DockGlobalElementModel::requestDockGroup(const QString &launcherFolderId)
{
    if (!groupIndex(launcherFolderId).isValid())
        return false;

    auto *settings = TaskManagerSettings::instance();
    if (settings->isDocked(launcherFolderId))
        return false;

    settings->appendDockedElement(launcherFolderId);
    return true;
}

void DockGlobalElementModel::groupItemsByApp()
{
    if (m_data.isEmpty())
        return;

    if (TaskManagerSettings::instance()->isWindowSplit())
        return;

    for (int i = 0; i < m_data.size(); ++i) {
        if (std::get<1>(m_data.at(i)) == nullptr)
            continue;
        const QString currentId = std::get<0>(m_data.at(i));

        int insertPos = i + 1;

        while (insertPos < m_data.size()
               && std::get<1>(m_data.at(insertPos)) != nullptr
               && std::get<0>(m_data.at(insertPos)) == currentId) {
            ++insertPos;
        }

        for (int j = insertPos; j < m_data.size(); ++j) {
            if (std::get<1>(m_data.at(j)) == nullptr || std::get<0>(m_data.at(j)) != currentId)
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
