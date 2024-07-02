// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QQmlEngine>
#include <QSize>
#include <QPoint>

namespace docktray {

class TrayItemPositionRegisterAttachedType : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int visualIndex MEMBER m_visualIndex NOTIFY visualIndexChanged)
    Q_PROPERTY(QSize visualSize MEMBER m_visualSize NOTIFY visualSizeChanged)
    Q_PROPERTY(QPoint visualPosition READ visualPosition NOTIFY visualPositionChanged)
    QML_ANONYMOUS
public:
    TrayItemPositionRegisterAttachedType(QObject *parent);

    QPoint visualPosition() const;

signals:
    void visualIndexChanged(int);
    void visualSizeChanged(QSize);
    void visualPositionChanged();

private:
    void registerVisualSize();

    int m_visualIndex = -1;
    QSize m_visualSize;
};

// -------------------------------------------------------

class TrayItemPositionRegister : public QObject
{
    Q_OBJECT
    QML_ATTACHED(TrayItemPositionRegisterAttachedType)
    QML_ELEMENT
public:
    static TrayItemPositionRegisterAttachedType *qmlAttachedProperties(QObject *object)
    {
        return new TrayItemPositionRegisterAttachedType(object);
    }
};

}
