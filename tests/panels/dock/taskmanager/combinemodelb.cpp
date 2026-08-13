// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "combinemodelb.h"

DataB::DataB(int id, TestModelB* model)
    : m_model(model)
    , m_id(id)
{
}

DataB::DataB(int id, const QString &data, TestModelB* model)
    : DataB(id, model)
{
    m_data = data;
}

int DataB::id()
{
    return m_id;
}

QString DataB::data()
{
    return m_data;
}

void DataB::setData(const QString &data)
{
    if (data == m_data) return;
    m_data = data;

    for (int row = 0; row < m_model->rowCount(); ++row) {
        if (m_model->index(row, 0).data(TestModelB::idRole).toInt() == m_id) {
            Q_EMIT m_model->dataChanged(m_model->index(row, 0), m_model->index(row, 0), {TestModelB::dataRole});
            break;
        }
    }
}

TestModelB::TestModelB(QObject *parent)
    : QAbstractListModel(parent)
{

}

TestModelB::~TestModelB()
{
    qDeleteAll(m_list);
    m_list.clear();
}

QHash<int, QByteArray> TestModelB::roleNames() const
{
    return {
        {idRole, "bId"},
        {dataRole, "bData"}
    };
}

int TestModelB::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_list.size();
}

QVariant TestModelB::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_list.size()) return {};
    switch (role) {
        case idRole:
            return m_list[index.row()]->id();
        case dataRole:
            return m_list[index.row()]->data();
    }

    return {};
}

void TestModelB::addData(DataB *data)
{
    beginInsertRows(QModelIndex(), m_list.size(), m_list.size());
    m_list.append(data);
    endInsertRows();
}

bool TestModelB::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() >= m_list.size())
        return false;

    if (role != dataRole)
        return false;

    m_list[index.row()]->setData(value.toString());
    return true;
}

void TestModelB::removeData(DataB *data)
{
    auto pos = m_list.indexOf(data);
    if (pos == -1) return;

    beginRemoveRows(QModelIndex(), pos, pos);
    m_list.removeAt(pos);
    delete data;
    endRemoveRows();
}
