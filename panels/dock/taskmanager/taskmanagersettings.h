// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dsglobal.h"

#include <QObject>
#include <DConfig>
#include <QScopedPointer>
#include <QJsonArray>

DCORE_USE_NAMESPACE


namespace dock {

class TaskManagerSettings : public QObject
{
    Q_OBJECT

public:
    static TaskManagerSettings* instance();

    bool isAllowedForceQuit();
    void setAllowedForceQuit(bool allowed);

    bool showAttentionAnimation() const;

    // The DConfig value of "noTaskGrouping" (window split option in control center).
    bool windowSplitValue() const;
    // The effective split state: the DConfig value is not applied in fashion mode.
    bool isWindowSplit();
    void setWindowSplit(bool split);

    // Fashion mode (Item_Alignment == "fashion") disables the window split feature.
    void setFashionMode(bool fashionMode);
    bool fashionMode() const;

    bool cgroupsBasedGrouping() const;
    QStringList cgroupsBasedGroupingSkipIds() const;
    QStringList cgroupsBasedGroupingSkipCategories() const;

    QStringList windowIconWhitelist() const;

    void setDockedElements(const QStringList &elements);
    void toggleDockedElement(const QString &element);
    void appendDockedElement(const QString &element);
    void removeDockedElement(const QString &element);
    QStringList dockedElements() const;
    bool isDocked(const QString &elementId) const;
    bool dockedApplicationsEnabled() const;

    void logMergeAppModel(bool mergeAppModelOn);

private:
    explicit TaskManagerSettings(QObject *parent = nullptr);
    inline void migrateFromDockedItems();
    inline void saveDockedElements();

Q_SIGNALS:
    void allowedForceQuitChanged();
    void showAttentionAnimationChanged();
    void windowSplitChanged();
    void fashionModeChanged();
    void dockedItemsChanged();
    void dockedElementsChanged();
    void dockedApplicationsEnabledChanged(bool enabled);

private:
    DConfig* m_taskManagerDconfig;

    bool m_allowForceQuit;
    bool m_showAttentionAnimation;
    bool m_windowSplit;
    bool m_fashionMode = false;
    bool m_cgroupsBasedGrouping;
    bool m_dockedApplicationsEnabled;
    QStringList m_dockedElements;
    QStringList m_windowIconWhitelist;
    QStringList m_cgroupsBasedGroupingSkipAppIds;
    QStringList m_cgroupsBasedGroupingSkipCategories;
};
}
