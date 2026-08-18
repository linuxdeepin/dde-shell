// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "appitemmodel.h"
#include "objectmanager1interface.h"

namespace apps
{
class AMAppItem;
class TrashMonitor;
class AMAppItemModel : public AppItemModel
{
    Q_OBJECT
    Q_PROPERTY(bool ready MEMBER m_ready READ ready NOTIFY readyChanged)
public:
    explicit AMAppItemModel(QObject *parent = nullptr);

    AMAppItem * appItem(const QString &id);

    bool ready() const;

signals:
    void readyChanged(bool);

private:
    void updateTrashIcon();

    bool m_ready;
    ObjectManager *m_manager;
    TrashMonitor *m_trashMonitor;
};
}