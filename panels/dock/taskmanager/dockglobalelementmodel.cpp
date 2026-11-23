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
#include <QSet>

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
    connect(TaskManagerSettings::instance(), &TaskManagerSettings::dockedFoldersEnabledChanged, this, &DockGlobalElementModel::loadDockedElements);

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
                    data = std::make_tuple(std::get<0>(data), std::get<1>(data), std::get<2>(data) - ((last - first) + 1), std::get<3>(data));
                }
            });
        },
        Qt::QueuedConnection);

    // Update sourceRow synchronously: readyChanged can load pins immediately
    // after a batch of app insertions. Queued insertion notifications would
    // then shift those already-current rows again and lose their actions.
    connect(
        m_appsModel,
        &QAbstractItemModel::rowsInserted,
        this,
        [this](const QModelIndex &parent, int first, int last) {
            Q_UNUSED(parent)
            const int insertedCount = (last - first) + 1;
            std::for_each(m_data.begin(), m_data.end(), [this, first, insertedCount](auto &data) {
                if (std::get<1>(data) == m_appsModel && std::get<2>(data) >= first) {
                    data = std::make_tuple(std::get<0>(data), std::get<1>(data), std::get<2>(data) + insertedCount, std::get<3>(data));
                }
            });

            for (auto &data : m_data) {
                if (std::get<1>(data) != m_appsModel || std::get<2>(data) != -1)
                    continue;
                const auto id = std::get<0>(data);
                auto res = m_appsModel->match(m_appsModel->index(0, 0), TaskManager::DesktopIdRole, id, 1, Qt::MatchExactly);
                std::get<2>(data) = res.isEmpty() ? -1 : res.first().row();
            }
        });

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

    // Structural signals only schedule a reconciliation. Their row numbers are
    // already obsolete if another window is inserted or removed before delivery.
    connect(m_activeAppModel, &QAbstractItemModel::rowsInserted, this, &DockGlobalElementModel::scheduleWindowSync);
    connect(m_activeAppModel, &QAbstractItemModel::rowsRemoved, this, &DockGlobalElementModel::scheduleWindowSync);
    connect(m_activeAppModel, &QAbstractItemModel::modelReset, this, &DockGlobalElementModel::scheduleWindowSync);
    connect(m_activeAppModel, &QAbstractItemModel::layoutChanged, this, &DockGlobalElementModel::scheduleWindowSync);
    connect(m_activeAppModel, &QAbstractItemModel::rowsMoved, this, &DockGlobalElementModel::scheduleWindowSync);
    connect(m_activeAppModel, &QAbstractItemModel::dataChanged, this,
        [this](const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &) {
            for (int row = 0; row < m_data.size(); ++row) {
                if (std::get<1>(m_data.at(row)) != m_activeAppModel)
                    continue;
                const auto source = sourceIndex(m_data.at(row));
                if (!source.isValid() || source.row() < topLeft.row() || source.row() > bottomRight.row())
                    continue;
                if (std::get<0>(m_data.at(row)) != source.data(TaskManager::DesktopIdRole).toString()) {
                    scheduleWindowSync();
                    continue;
                }
                // DockCombineModel translates roles in data(); forwarding its
                // underlying numeric role ids can leave the displayed icon stale.
                Q_EMIT dataChanged(index(row, 0), index(row, 0));
            }
        });

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
    scheduleWindowSync();
}

DockGlobalElementModel::Item DockGlobalElementModel::makeItem(const QString &id, QAbstractItemModel *model, int row) const
{
    const auto winId = model == m_activeAppModel
        ? model->index(row, 0).data(TaskManager::WinIdRole).toString() : QString();
    return std::make_tuple(id, model, row, winId);
}

// 目前需要 identify 符合才能返回。
QModelIndex DockGlobalElementModel::sourceIndex(const Item &item) const
{
    auto model = std::get<1>(item);
    if (!model)
        return {};
    const auto role = model == m_activeAppModel ? TaskManager::WinIdRole : TaskManager::DesktopIdRole;
    const auto identity = model == m_activeAppModel ? std::get<3>(item) : std::get<0>(item);
    if (identity.isEmpty())
        return {};
    const auto row = std::get<2>(item);
    const auto cached = model->index(row, 0);
    if (cached.isValid() && cached.data(role).toString() == identity)
        return cached;
    return model->match(model->index(0, 0), role, identity, 1, Qt::MatchExactly).value(0);
}

void DockGlobalElementModel::scheduleWindowSync()
{
    if (m_windowSyncPending)
        return;
    m_windowSyncPending = true;
    QMetaObject::invokeMethod(this, [this] {
        m_windowSyncPending = false;
        syncWindows();
    }, Qt::QueuedConnection);
}

void DockGlobalElementModel::syncWindows()
{
    //pengding是保存了所有有效窗口， 后面开始匹配，成功匹配就从pending中删除，最后剩下的就是新增的窗口
    QList<Item> pending;
    for (int row = 0; row < m_activeAppModel->rowCount(); ++row) {
        const auto source = m_activeAppModel->index(row, 0);
        const auto id = source.data(TaskManager::DesktopIdRole).toString();
        const auto winId = source.data(TaskManager::WinIdRole).toString();
        if (!id.isEmpty() && !winId.isEmpty())
            pending.append(makeItem(id, m_activeAppModel, row));
    }

    QSet<QString> retainedWindows;
    bool identityChanged = false;
    // Refresh surviving windows first, preserving the user's visual ordering.
    for (auto &item : m_data) {
        if (std::get<1>(item) != m_activeAppModel)
            continue;
        const auto found = std::find_if(pending.begin(), pending.end(), [&item](const Item &candidate) {
            return std::get<3>(candidate) == std::get<3>(item);
        });
        if (found == pending.end())
            continue;
        identityChanged |= std::get<0>(item) != std::get<0>(*found);
        item = *found;
        retainedWindows.insert(std::get<3>(item));
        pending.erase(found);
    }

    for (int pos = 0; pos < m_data.size();) {
        auto &item = m_data[pos];
        const auto model = std::get<1>(item);
        const auto id = std::get<0>(item);
        //1. 文件夹 已确认存在的窗口 保留在任务栏显示
        if (!model || (model == m_activeAppModel && retainedWindows.contains(std::get<3>(item)))) {
            ++pos;
            continue;
        }
        //2. 给固定图标或者关闭的窗口 找到同应用的新窗口
        const auto replacement = std::find_if(pending.begin(), pending.end(), [&id](const Item &candidate) {
            return std::get<0>(candidate) == id;
        });
        if (replacement != pending.end()) {
            // Reuse a pin or a closed splash window's position for the new window.
            item = *replacement;
            retainedWindows.insert(std::get<3>(item));
            pending.erase(replacement);
            ++pos;
            continue;
        }
        //3.没有新窗口接替的固定图标，继续保留任务栏中显示
        if (model == m_appsModel) {
            ++pos;
            continue;
        }
        //4.下面判断同一个应用，在任务栏里是否还有有效的窗口条目，或者已经保留下来的固定入口
        const bool hasOtherItem = std::any_of(m_data.cbegin(), m_data.cend(), [&](const Item &candidate) {
            return std::get<0>(candidate) == id
                && (std::get<1>(candidate) == m_appsModel
                    || (std::get<1>(candidate) == m_activeAppModel && retainedWindows.contains(std::get<3>(candidate))));
        });
        if (!hasOtherItem && m_dockedElements.contains(std::make_tuple(QStringLiteral("desktop"), id))) {
            const auto app = m_appsModel->match(m_appsModel->index(0, 0), TaskManager::DesktopIdRole,
                                                id, 1, Qt::MatchExactly).value(0);
            if (app.isValid()) {
                item = makeItem(id, m_appsModel, app.row());
                ++pos;
                continue;
            }
        }
        beginRemoveRows({}, pos, pos);
        m_data.removeAt(pos);
        endRemoveRows();
    }
    //5. 把 pending 中剩余的新窗口插入任务栏
    for (const auto &item : std::as_const(pending)) {
        int pos = m_data.size();
        for (int row = m_data.size() - 1; row >= 0; --row) {
            if (std::get<0>(m_data.at(row)) == std::get<0>(item)) {
                pos = row + 1;
                break;
            }
        }
        beginInsertRows({}, pos, pos);
        m_data.insert(pos, item);
        endInsertRows();
    }

    if (!m_data.isEmpty()) {
        // Replacements change the window id as well as the icon/title and menus.
        // Notify only after every entry refers to the current source state.
        const auto roles = identityChanged ? roleNames().keys() + QList<int>{TaskManager::DesktopIdRole} : QList<int>();
        Q_EMIT dataChanged(index(0, 0), index(m_data.size() - 1, 0), roles);
    }
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
            if (folder && !TaskManagerSettings::instance()->dockedFoldersEnabled()) continue;
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
                m_data.insert(position, makeItem(id, model, row));
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
    auto row = sourceIndex(data).row();

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
    auto row = sourceIndex(data).row();

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

    case TaskManager::DockedRole:
        return model == nullptr || m_dockedElements.contains(std::make_tuple("desktop", id));

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
    auto sourceRow = sourceIndex(data).row();
    if (sourceRow < 0)
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
    auto sourceRow = sourceIndex(data).row();
    if (sourceRow < 0)
        return;

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
    auto sourceRow = sourceIndex(data).row();
    if (sourceRow < 0)
        return;

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
