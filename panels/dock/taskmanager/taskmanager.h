// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dsglobal.h"
#include "itemmodel.h"
#include "containment.h"
#include "abstractwindow.h"
#include "abstractwindowmonitor.h"

#include <QPointer>

DS_BEGIN_NAMESPACE
namespace dock {
class AppItem;

class TaskManager : public DContainment
{
    Q_OBJECT
    Q_PROPERTY(ItemModel* dataModel READ dataModel NOTIFY itemsChanged)

    Q_PROPERTY(bool windowSplit READ windowSplit NOTIFY windowSplitChanged)
    Q_PROPERTY(bool allowForceQuit READ allowForceQuit NOTIFY allowedForceQuitChanged)

public:
    explicit TaskManager(QObject* parent = nullptr);

    QStringList getDockedItems();

    ItemModel* dataModel();

    virtual bool init() override;
    virtual bool load() override;

    bool windowSplit();
    bool allowForceQuit();

    Q_INVOKABLE void clickItem(const QString& itemid, const QString& menuId);
    Q_INVOKABLE void showItemPreview(const QString& itemId, QObject* relativePositionItem, int32_t previewXoffset, int32_t previewYoffset, uint32_t direction);
    Q_INVOKABLE void hideItemPreview();

Q_SIGNALS:
    void itemsChanged();
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

