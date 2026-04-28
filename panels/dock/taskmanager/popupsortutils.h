// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QList>
#include <QString>
#include <QVariantMap>
#include <Qt>

namespace dock {

enum class PopupSortField {
    Name,
    ModifiedTime,
    CreatedTime,
    Size,
    Type,
};

struct PopupSortState {
    PopupSortField field = PopupSortField::Name;
    Qt::SortOrder order = Qt::AscendingOrder;
};

struct PopupSortableEntry {
    QVariantMap entryData;
    QString name;
    QString typeText;
    qint64 modifiedTime = 0;
    qint64 createdTime = 0;
    qint64 size = 0;
    bool directory = false;
};

PopupSortField popupSortFieldFromString(const QString &fieldName);
QString popupSortFieldToString(PopupSortField field);
PopupSortState cyclePopupSortState(const PopupSortState &currentState, PopupSortField selectedField);
void sortPopupEntries(QList<PopupSortableEntry> *entries, const PopupSortState &state, bool keepDirectoriesFirst);

}
