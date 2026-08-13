// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "combinemodela.h"

DataA::DataA(int id, TestModelA* model)
    : m_model(model)
    , m_id(id)
{
}

DataA::DataA(int id, const QString &data, TestModelA* model)
    : DataA(id, model)
{
    m_data = data;
}

int DataA::id()
{
    return m_id;
}

QString DataA::data()
{
    return m_data;
}

void DataA::setData(const QString &data)
{
    if (data == m_data) return;
    m_data = data;

    for (int row = 0; row < m_model->rowCount(); ++row) {
        if (m_model->index(row, 0).data(TestModelA::idRole).toInt() == m_id) {
            Q_EMIT m_model->dataChanged(m_model->index(row, 0), m_model->index(row, 0), {TestModelA::dataRole});
            break;
        }
    }
}

TestModelA::TestModelA(QObject *parent)
    : QAbstractListModel(parent)
{

}

TestModelA::~TestModelA()
{
    qDeleteAll(m_list);
    m_list.clear();
}

QHash<int, QByteArray> TestModelA::roleNames() const
{
    return {
        {idRole, "aId"},
        {dataRole, "aData"}
    };
}

int TestModelA::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_list.size();
}

QVariant TestModelA::data(const QModelIndex &index, int role) const
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

void TestModelA::addData(DataA *data)
{
    beginInsertRows(QModelIndex(), m_list.size(), m_list.size());
    m_list.append(data);
    endInsertRows();
}

bool TestModelA::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() >= m_list.size())
        return false;

    if (role != dataRole)
        return false;

    m_list[index.row()]->setData(value.toString());
    return true;
}

void TestModelA::removeData(DataA *data)
{
    auto pos = m_list.indexOf(data);
    if (pos == -1) return;

    beginRemoveRows(QModelIndex(), pos, pos);
    m_list.removeAt(pos);
    delete data;
    endRemoveRows();
}
