// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "applet.h"
#include "dsglobal.h"
#include "appitemmodel.h"
#include "abstractwindow.h"
#include "abstractwindowmonitor.h"

#include <QPointer>

DS_BEGIN_NAMESPACE
namespace dock {
class AppItem;

class TaskManager : public DApplet
{
    Q_OBJECT
    Q_PROPERTY(AppItemModel* dataModel READ dataModel NOTIFY appItemsChanged)

    Q_PROPERTY(bool windowSplit READ windowSplit NOTIFY windowSplitChanged)
    Q_PROPERTY(bool allowForceQuit READ allowForceQuit NOTIFY allowedForceQuitChanged)

public:
    explicit TaskManager(QObject* parent = nullptr);

    QStringList getDockedItems();

    AppItemModel* dataModel();

    virtual bool init() override;
    virtual bool load(const DAppletData &data) override;

    bool windowSplit();
    bool allowForceQuit();

    Q_INVOKABLE void clickItem(const QString& itemid);
    Q_INVOKABLE void clickItemMenu(const QString& itemId, const QString& menuId);

Q_SIGNALS:
    void appItemsChanged();
    void windowSplitChanged();
    void allowedForceQuitChanged();

private Q_SLOTS:
    void handleWindowAdded(QPointer<AbstractWindow> window);

private:
    void loadDockedAppItems();

private:
    QPointer<AppItem> m_currentActiveItem;                  // currnet activate item
    QScopedPointer<AbstractWindowMonitor> m_windowMonitor;
};

}
DS_END_NAMESPACE

