// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>
#include <QVariant>
#include <QList>

// Minimal QAbstractListModel that provides WinIdRole and DesktopIdRole data,
// used as a source model for HoverPreviewProxyModel tests.

struct SourceRow {
    uint32_t winId;
    QString desktopId;
};

class TestSourceModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        WinIdRole = Qt::UserRole + 1,
        DesktopIdRole = 0x1000,
    };
    Q_ENUM(Roles)

    explicit TestSourceModel(QObject *parent = nullptr) : QAbstractListModel(parent) {}

    QHash<int, QByteArray> roleNames() const override
    {
        return { {WinIdRole, "winId"}, {DesktopIdRole, "desktopId"} };
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        Q_UNUSED(parent)
        return m_rows.size();
    }

    QVariant data(const QModelIndex &index, int role) const override
    {
        if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size())
            return {};
        switch (role) {
        case WinIdRole:
            return m_rows[index.row()].winId;
        case DesktopIdRole:
            return m_rows[index.row()].desktopId;
        }
        return {};
    }

    void addRow(uint32_t winId, const QString &desktopId)
    {
        beginInsertRows(QModelIndex(), m_rows.size(), m_rows.size());
        m_rows.append({winId, desktopId});
        endInsertRows();
    }

    void clear()
    {
        beginResetModel();
        m_rows.clear();
        endResetModel();
    }

private:
    QList<SourceRow> m_rows;
};
