// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "popupsortutils.h"

#include <QCollator>

#include <algorithm>

namespace dock {
namespace {

int compareText(const QString &left, const QString &right)
{
    static thread_local QCollator collator;
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    collator.setNumericMode(true);
    collator.setIgnorePunctuation(false);
    return collator.compare(left, right);
}

int compareNumbers(qint64 left, qint64 right)
{
    if (left < right) {
        return -1;
    }
    if (left > right) {
        return 1;
    }
    return 0;
}

int compareEntries(const PopupSortableEntry &left, const PopupSortableEntry &right, PopupSortField field)
{
    switch (field) {
    case PopupSortField::ModifiedTime:
        return compareNumbers(left.modifiedTime, right.modifiedTime);
    case PopupSortField::CreatedTime:
        return compareNumbers(left.createdTime, right.createdTime);
    case PopupSortField::Size:
        return compareNumbers(left.size, right.size);
    case PopupSortField::Type: {
        const int typeCompare = compareText(left.typeText, right.typeText);
        if (typeCompare != 0) {
            return typeCompare;
        }
        return compareText(left.name, right.name);
    }
    case PopupSortField::Name:
    default:
        return compareText(left.name, right.name);
    }
}

}

PopupSortField popupSortFieldFromString(const QString &fieldName)
{
    if (fieldName == QStringLiteral("modified")) {
        return PopupSortField::ModifiedTime;
    }
    if (fieldName == QStringLiteral("created")) {
        return PopupSortField::CreatedTime;
    }
    if (fieldName == QStringLiteral("size")) {
        return PopupSortField::Size;
    }
    if (fieldName == QStringLiteral("type")) {
        return PopupSortField::Type;
    }
    return PopupSortField::Name;
}

QString popupSortFieldToString(PopupSortField field)
{
    switch (field) {
    case PopupSortField::ModifiedTime:
        return QStringLiteral("modified");
    case PopupSortField::CreatedTime:
        return QStringLiteral("created");
    case PopupSortField::Size:
        return QStringLiteral("size");
    case PopupSortField::Type:
        return QStringLiteral("type");
    case PopupSortField::Name:
    default:
        return QStringLiteral("name");
    }
}

PopupSortState cyclePopupSortState(const PopupSortState &currentState, PopupSortField selectedField)
{
    PopupSortState nextState = currentState;
    if (nextState.field == selectedField) {
        nextState.order = nextState.order == Qt::AscendingOrder ?
                              Qt::DescendingOrder :
                              Qt::AscendingOrder;
        return nextState;
    }

    nextState.field = selectedField;
    nextState.order = Qt::AscendingOrder;
    return nextState;
}

void sortPopupEntries(QList<PopupSortableEntry> *entries, const PopupSortState &state, bool keepDirectoriesFirst)
{
    if (!entries || entries->size() < 2) {
        return;
    }

    std::stable_sort(entries->begin(), entries->end(), [state, keepDirectoriesFirst](const PopupSortableEntry &left,
                                                                                      const PopupSortableEntry &right) {
        if (keepDirectoriesFirst && left.directory != right.directory) {
            return left.directory && !right.directory;
        }

        int result = compareEntries(left, right, state.field);
        if (result == 0 && state.field != PopupSortField::Name) {
            result = compareText(left.name, right.name);
        }

        if (result == 0) {
            return false;
        }

        return state.order == Qt::AscendingOrder ? result < 0 : result > 0;
    });
}

}
