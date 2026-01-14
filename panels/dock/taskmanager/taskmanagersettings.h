// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
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

    bool isWindowSplit();
    void setWindowSplit(bool split);

    bool cgroupsBasedGrouping() const;
    QStringList cgroupsBasedGroupingSkipIds() const;
    QStringList cgroupsBasedGroupingSkipCategories() const;

    void setDockedElements(const QStringList &elements);
    void toggleDockedElement(const QString &element);
    void appendDockedElement(const QString &element);
    void removeDockedElement(const QString &element);
    QStringList dockedElements() const;
    bool isDocked(const QString &elementId) const;

private:
    explicit TaskManagerSettings(QObject *parent = nullptr);
    inline void migrateFromDockedItems();
    inline void saveDockedElements();

Q_SIGNALS:
    void allowedForceQuitChanged();
    void windowSplitChanged();
    void dockedItemsChanged();
    void dockedElementsChanged();

private:
    DConfig* m_taskManagerDconfig;

    bool m_allowForceQuit;
    bool m_windowSplit;
    bool m_cgroupsBasedGrouping;
    QStringList m_dockedElements;
    QStringList m_cgroupsBasedGroupingSkipAppIds;
    QStringList m_cgroupsBasedGroupingSkipCategories;
};
}
