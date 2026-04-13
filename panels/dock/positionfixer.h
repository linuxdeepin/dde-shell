// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QQuickItem>
#include <QTimer>
#include <QtQml/qqml.h>

namespace dock {

class PositionFixer : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QQuickItem *item READ item WRITE setItem NOTIFY itemChanged)
    Q_PROPERTY(QQuickItem *container READ container WRITE setContainer NOTIFY containerChanged)

    QML_NAMED_ELEMENT(PositionFixer)

public:
    explicit PositionFixer(QQuickItem *parent = nullptr);

    QQuickItem *item() const;
    void setItem(QQuickItem *newItem);

    QQuickItem *container() const;
    void setContainer(QQuickItem *newContainer);

    Q_INVOKABLE void fix();
    Q_INVOKABLE void forceFix();

signals:
    void itemChanged();
    void containerChanged();
    void useZeroTargetChanged();

private:
    QQuickItem *m_item = nullptr;
    QQuickItem *m_container = nullptr;
    QTimer *m_timer = nullptr;
};

}
