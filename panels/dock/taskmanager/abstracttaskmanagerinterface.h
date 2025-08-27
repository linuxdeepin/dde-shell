// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractProxyModel>
#include <QModelIndex>
#include <algorithm>
#include <utility>

namespace dock
{
class AbstractTaskManagerInterface
{
protected:
    template<typename Func, typename... Args>
    void callInterfaceMethod(const QModelIndex &index, Func func, Args &&...args) const
    {
        if (nullptr == m_model) {
            return;
        }
        auto interface = dynamic_cast<AbstractTaskManagerInterface *>(m_model->sourceModel());
        if (interface != nullptr) {
            (interface->*func)(m_model->mapToSource(index), std::forward<Args>(args)...);
        }
    }

    template<typename Func, typename... Args>
    void callInterfaceMethod(const QModelIndexList &indexes, Func func, Args &&...args) const
    {
        if (nullptr == m_model) {
            return;
        }
        QModelIndexList sourceModelIndexes;
        std::transform(indexes.begin(), indexes.end(), std::back_inserter(sourceModelIndexes), [this](const auto &index) {
            return m_model->mapToSource(index);
        });
        auto interface = dynamic_cast<AbstractTaskManagerInterface *>(m_model->sourceModel());
        if (interface != nullptr) {
            (interface->*func)(sourceModelIndexes, std::forward<Args>(args)...);
        }
    }

public:
    AbstractTaskManagerInterface(QAbstractProxyModel *model)
        : m_model(model){};

    virtual void requestActivate(const QModelIndex &index) const
    {
        callInterfaceMethod(index, &AbstractTaskManagerInterface::requestActivate);
    }
    virtual void requestNewInstance(const QModelIndex &index, const QString &action) const
    {
        callInterfaceMethod(index, &AbstractTaskManagerInterface::requestNewInstance, action);
    }
    virtual void requestOpenUrls(const QModelIndex &index, const QList<QUrl> &urls) const
    {
        callInterfaceMethod(index, &AbstractTaskManagerInterface::requestOpenUrls, urls);
    }
    virtual void requestClose(const QModelIndex &index, bool force = false) const
    {
        callInterfaceMethod(index, &AbstractTaskManagerInterface::requestClose, force);
    }
    virtual void requestUpdateWindowGeometry(const QModelIndex &index, const QRect &geometry, QObject *delegate = nullptr) const
    {
        callInterfaceMethod(index, &AbstractTaskManagerInterface::requestUpdateWindowGeometry, geometry, delegate);
    }
    virtual void requestWindowsView(const QModelIndexList &indexes) const
    {
        callInterfaceMethod(indexes, &AbstractTaskManagerInterface::requestWindowsView);
    }

protected:
    QAbstractProxyModel *m_model;
};
}
