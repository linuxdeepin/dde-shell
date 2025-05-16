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
public:
    virtual void requestActivate(const QModelIndex &index) const = 0;
    virtual void requestNewInstance(const QModelIndex &index, const QString &action) const = 0;
    virtual void requestOpenUrls(const QModelIndex &index, const QList<QUrl> &urls) const = 0;
    virtual void requestClose(const QModelIndex &index, bool force = false) const = 0;
    virtual void requestUpdateWindowGeometry(const QModelIndex &index, const QRect &geometry, QObject *delegate = nullptr) const = 0;

    virtual void
    requestPreview(const QModelIndexList &indexes, QObject *relativePositionItem, int32_t previewXoffset, int32_t previewYoffset, uint32_t direction) const = 0;
    virtual void requestWindowsView(const QModelIndexList &indexes) const = 0;
};

template<typename Func, typename... Args>
void callInterfaceMethod(const QAbstractProxyModel *model, const QModelIndex &index, Func func, Args &&...args)
{
    if (nullptr == model) {
        return;
    }
    auto interface = dynamic_cast<AbstractTaskManagerInterface *>(model->sourceModel());
    if (interface != nullptr) {
        (interface->*func)(model->mapToSource(index), std::forward<Args>(args)...);
    }
}

template<typename Func, typename... Args>
void callInterfaceMethod(const QAbstractProxyModel *model, const QModelIndexList &indexes, Func func, Args &&...args)
{
    if (nullptr == model) {
        return;
    }
    QModelIndexList sourceModelIndexes;
    std::transform(indexes.begin(), indexes.end(), std::back_inserter(sourceModelIndexes), [model](const auto &index) {
        return model->mapToSource(index);
    });
    auto interface = dynamic_cast<AbstractTaskManagerInterface *>(model->sourceModel());
    if (interface != nullptr) {
        (interface->*func)(sourceModelIndexes, std::forward<Args>(args)...);
    }
}
}
