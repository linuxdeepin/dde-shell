// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QThread>
#include "xcbgetinfo.h"
#include <QMetaType>

namespace dock {
class XcbThread : public QThread
{
    Q_OBJECT

public:
    explicit XcbThread(QObject *parent = nullptr);
    ~XcbThread();

signals:
    void windowInfoChanged(const QString &name, uint id);
    void windowDestroyChanged(uint id);
    void windowInfoChangedForeground(const QString &name, uint id);
    void windowInfoChangedBackground(const QString &name, uint id);
protected:
    void run() override;

private:
    XcbGetInfo xcbGetInfo;
};
}
