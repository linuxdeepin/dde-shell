// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "abstracttaskmanagerinterface.h"
#include "abstractwindow.h"
#include "containment.h"
#include "dockcombinemodel.h"
#include "dockglobalelementmodel.h"
#include "dockitemmodel.h"

#include <QPointer>

namespace dock {
class AppItem;
class AbstractWindowMonitor;
class TaskManager : public DS_NAMESPACE::DContainment, public AbstractTaskManagerInterface
{
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel *dataModel READ dataModel NOTIFY dataModelChanged)

    Q_PROPERTY(bool windowSplit READ windowSplit NOTIFY windowSplitChanged)
    Q_PROPERTY(bool windowFullscreen READ windowFullscreen NOTIFY windowFullscreenChanged)
    Q_PROPERTY(bool allowForceQuit READ allowForceQuit NOTIFY allowedForceQuitChanged)

public:
    enum Roles {
        // abstract window
        WinIdRole = Qt::UserRole + 1,
        PidRole,
        IdentityRole,
        WinIconRole,
        WinTitleRole,
        ActiveRole,
        ShouldSkipRole,
        AttentionRole,

        // item
        ItemIdRole,
        MenusRole,
        WindowsRole,

        // from dde-apps
        DesktopIdRole = 0x1000,
        NameRole,
        IconNameRole,
        StartUpWMClassRole,
        NoDisplayRole,
        ActionsRole,
        DDECategoryRole,
        InstalledTimeRole,
        LastLaunchedTimeRole,
        LaunchedTimesRole,
        DockedRole,
        OnDesktopRole,
        AutoStartRole,
        AppTypeRole,
    };
    Q_ENUM(Roles)

    explicit TaskManager(QObject* parent = nullptr);

    DockItemModel *dataModel() const;

    virtual bool init() override;
    virtual bool load() override;

    bool windowSplit();
    bool windowFullscreen();
    bool allowForceQuit();

    Q_INVOKABLE void requestActivate(const QModelIndex &index) const override;
    Q_INVOKABLE void requestNewInstance(const QModelIndex &index, const QString &action = QString()) const override;

    Q_INVOKABLE void requestOpenUrls(const QModelIndex &index, const QList<QUrl> &urls) const override;
    Q_INVOKABLE void requestClose(const QModelIndex &index, bool force = false) const override;
    Q_INVOKABLE void requestUpdateWindowGeometry(const QModelIndex &index, const QRect &geometry, QObject *delegate = nullptr) const override;
    Q_INVOKABLE void requestPreview(const QModelIndexList &indexes,
                                    QObject *relativePositionItem,
                                    int32_t previewXoffset,
                                    int32_t previewYoffset,
                                    uint32_t direction) const override;
    Q_INVOKABLE void requestWindowsView(const QModelIndexList &indexes) const override;

    Q_INVOKABLE QString desktopIdToAppId(const QString& desktopId);
    Q_INVOKABLE bool requestDockByDesktopId(const QString& desktopID);
    Q_INVOKABLE bool requestUndockByDesktopId(const QString& desktopID);
    Q_INVOKABLE bool RequestDock(QString appID);
    Q_INVOKABLE bool IsDocked(QString appID);
    Q_INVOKABLE bool RequestUndock(QString appID);

    Q_INVOKABLE void dropFilesOnItem(const QString &itemId, const QStringList &urls);
    Q_INVOKABLE void hideItemPreview();

    Q_INVOKABLE void setAppItemWindowIconGeometry(const QString& appid, QObject* relativePositionItem, const int& x1, const int& y1, const int& x2, const int& y2);

Q_SIGNALS:
    void dataModelChanged();
    void windowSplitChanged();
    void windowFullscreenChanged(bool);
    void allowedForceQuitChanged();

private Q_SLOTS:
    void handleWindowAdded(QPointer<AbstractWindow> window);

private:
    QScopedPointer<AbstractWindowMonitor> m_windowMonitor;
    bool m_windowFullscreen;
    DockCombineModel *m_activeAppModel = nullptr;
    DockGlobalElementModel *m_dockGlobalElementModel = nullptr;
    DockItemModel *m_itemModel = nullptr;
};

}

