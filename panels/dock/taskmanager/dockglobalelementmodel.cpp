// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
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
#include <QProcess>

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
    connect(m_appsModel, &QAbstractItemModel::rowsRemoved, this, [this](const QModelIndex &parent, int first, int last) {
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
    });

    connect(m_activeAppModel, &QAbstractItemModel::rowsInserted, this, [this](const QModelIndex &parent, int first, int last) {
        Q_UNUSED(parent)
        for (int i = first; i <= last; ++i) {
            auto index = m_activeAppModel->index(i, 0);
            auto desktopId = index.data(TaskManager::DesktopIdRole).toString();

            auto it = std::find_if(m_data.begin(), m_data.end(), [this, &desktopId](auto &data) {
                return m_appsModel == std::get<1>(data) && desktopId == std::get<0>(data);
            });

            if (it != m_data.end()) {
                *it = std::make_tuple(desktopId, m_activeAppModel, i);
                auto pIndex = this->index(it - m_data.begin(), 0);
                Q_EMIT dataChanged(pIndex, pIndex, {TaskManager::ActiveRole, TaskManager::AttentionRole, TaskManager::WindowsRole, TaskManager::MenusRole});

            } else {
                beginInsertRows(QModelIndex(), m_data.size(), m_data.size());
                m_data.append(std::make_tuple(desktopId, m_activeAppModel, first));
                endInsertRows();
            }
        }

        std::for_each(m_data.begin(), m_data.end(), [this, first, last](auto &data) {
            if (std::get<1>(data) == m_activeAppModel && std::get<2>(data) > first) {
                data = std::make_tuple(std::get<0>(data), std::get<1>(data), std::get<2>(data) + ((last - first) + 1));
            }
        });
    });

    connect(m_activeAppModel, &QAbstractItemModel::rowsRemoved, this, [this](const QModelIndex &parent, int first, int last) {
        Q_UNUSED(parent)
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
                auto pIndex = this->index(pos, 0);
                auto res = m_appsModel->match(m_appsModel->index(0, 0), TaskManager::DesktopIdRole, id, 1, Qt::MatchExactly);
                if (res.isEmpty()) {
                    beginRemoveRows(QModelIndex(), pos, pos);
                    m_data.remove(pos);
                    endRemoveRows();
                } else {
                    auto row = res.first().row();
                    *it = std::make_tuple(id, m_appsModel, row);
                    Q_EMIT dataChanged(pIndex, pIndex, {TaskManager::ActiveRole, TaskManager::AttentionRole, TaskManager::WindowsRole, TaskManager::MenusRole});
                }
            } else {
                beginRemoveRows(QModelIndex(), pos, pos);
                m_data.remove(pos);
                endRemoveRows();
            }
        }
        std::for_each(m_data.begin(), m_data.end(), [this, first, last](auto &data) {
            if (std::get<1>(data) == m_activeAppModel && std::get<2>(data) >= first) {
                data = std::make_tuple(std::get<0>(data), std::get<1>(data), std::get<2>(data) - ((last - first) + 1));
            }
        });
    });

    connect(m_activeAppModel,
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
            });

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

void DockGlobalElementModel::loadDockedElements()
{
    QList<std::tuple<QString, QString>> newDocked;
    for (auto elementInfo : TaskManagerSettings::instance()->dockedElements()) {
        auto pair = elementInfo.split('/');
        if (pair.size() != 2)
            continue;

        auto type = pair[0];
        auto id = pair[1];

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

        newDocked.append(tmp);
        if (m_dockedElements.contains(tmp))
            continue;

        auto isRunning = std::any_of(m_data.constBegin(), m_data.constEnd(), [this, &id](const auto &data) {
            return std::get<0>(data) == id;
        });

        if (!isRunning) {
            beginInsertRows(QModelIndex(), m_data.size(), m_data.size());
            m_data.append(std::make_tuple(id, model, row));
            endInsertRows();
        }
    }

    for (auto it = m_dockedElements.begin(); it < m_dockedElements.end(); ++it) {
        if (newDocked.contains(*it))
            continue;
        auto type = std::get<0>(*it), id = std::get<1>(*it);
        auto dataIt = std::find_if(m_data.begin(), m_data.end(), [this, &id](const auto &data) {
            return std::get<0>(data) == id && std::get<1>(data) == m_appsModel;
        });
        if (dataIt != m_data.end()) {
            auto pos = (dataIt - m_data.begin());
            beginRemoveRows(QModelIndex(), pos, pos);
            m_data.remove(pos);
            endRemoveRows();
        }
    }

    m_dockedElements = newDocked;

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

    QJsonArray menusArray;
    menusArray.append(QJsonObject{{"id", ""}, {"name", model == m_activeAppModel ? index.data(TaskManager::WinTitleRole).toString() : tr("Open")}});

    auto actions = model->index(row, 0).data(TaskManager::ActionsRole).toByteArray();
    for (auto action : QJsonDocument::fromJson(actions).array()) {
        menusArray.append(action);
    }

    bool isDocked = (model == nullptr) || m_dockedElements.contains(std::make_tuple("desktop", id));
    menusArray.append(QJsonObject{{"id", DOCK_ACTION_DOCK}, {"name", isDocked ? tr("Undock") : tr("Dock")}});

    if (model == m_activeAppModel) {
        if (TaskManagerSettings::instance()->isAllowedForceQuit()) {
            menusArray.append(QJsonObject{{"id", DOCK_ACTION_FORCEQUIT}, {"name", tr("Force Quit")}});
        }
        menusArray.append(QJsonObject{{"id", DOCK_ACTION_CLOSEALL}, {"name", tr("Close All")}});
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

    switch (role) {
    case TaskManager::ItemIdRole:
        return id;
    case TaskManager::WindowsRole: {
        if (model == m_activeAppModel) {
            return QStringList{model->index(row, 0).data(TaskManager::WinIdRole).toString()};
        }
        // For m_appsModel data, when it's GroupModel we can directly get all window IDs for this desktop ID
        QModelIndex groupIndex = model->index(row, 0);
        return groupIndex.data(TaskManager::WindowsRole).toStringList();
    }
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

    if (sourceModel == m_activeAppModel) {
        auto sourceIndex = sourceModel->index(sourceRow, 0);
        m_activeAppModel->requestActivate(sourceIndex);
    } else {
        this->requestNewInstance(index, "");
    }
}

void DockGlobalElementModel::requestOpenUrls(const QModelIndex &index, const QList<QUrl> &urls) const
{
    auto data = m_data.value(index.row());
    auto id = std::get<0>(data);

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

void DockGlobalElementModel::requestNewInstance(const QModelIndex &index, const QString &action) const
{
    if (action == DOCK_ACTION_DOCK) {
        auto data = m_data.value(index.row());
        auto id = std::get<0>(data);
        TaskManagerSettings::instance()->toggleDockedElement(QStringLiteral("desktop/%1").arg(id));
    } else if (action == DOCK_ACTION_FORCEQUIT) {
        requestClose(index, true);
    } else if (action == DOCK_ACTION_CLOSEALL) {
        requestClose(index);
    } else {
        auto data = m_data.value(index.row());
        auto id = std::get<0>(data);
        QProcess process;
        process.setProcessChannelMode(QProcess::MergedChannels);
        process.start("dde-am", {"--by-user", id, action});
        process.waitForFinished();
        return;
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
void DockGlobalElementModel::requestUpdateWindowGeometry(const QModelIndex &index, const QRect &geometry, QObject *delegate) const
{
    Q_UNUSED(index)
    Q_UNUSED(geometry)
    Q_UNUSED(delegate)
}

void DockGlobalElementModel::requestWindowsView(const QModelIndexList &indexes) const
{
    Q_UNUSED(indexes)
}
}
