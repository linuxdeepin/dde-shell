// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QPoint>
#include <QQmlEngine>
#include <QSize>

namespace docktray {

struct DropIndex {
    Q_GADGET
    Q_PROPERTY(int index MEMBER index)
    Q_PROPERTY(bool isOnItem MEMBER isOnItem)
    Q_PROPERTY(bool isBefore MEMBER isBefore)
    QML_ELEMENT
public:
    int index;
    bool isOnItem = true;
    bool isBefore = false;
};

class TrayItemPositionManager : public QObject
{
    Q_OBJECT
    // dock properties, to notify tray items its property has been changed
    Q_PROPERTY(Qt::Orientation orientation MEMBER m_orientation NOTIFY orientationChanged)
    Q_PROPERTY(int dockHeight MEMBER m_dockHeight NOTIFY dockHeightChanged)
    Q_PROPERTY(QSize itemVisualSize MEMBER m_itemVisualSize CONSTANT FINAL)
    Q_PROPERTY(int itemSpacing MEMBER m_itemSpacing CONSTANT FINAL)
    Q_PROPERTY(int itemPadding MEMBER m_itemPadding CONSTANT FINAL)
    QML_ELEMENT
    QML_SINGLETON
public:
    static TrayItemPositionManager &instance()
    {
        static TrayItemPositionManager _instance;
        return _instance;
    }

    static TrayItemPositionManager *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine)
    {
        Q_UNUSED(qmlEngine)
        Q_UNUSED(jsEngine)
        return &instance();
    }

    Qt::Orientation orientation() const;
    int dockHeight() const;

signals:
    void orientationChanged(Qt::Orientation);
    void dockHeightChanged(int);
    void visualSizeChanged(QSize);
    void visualItemCountChanged(int);
    void visualItemSizeChanged();

private:
    explicit TrayItemPositionManager(QObject *parent = nullptr);

    Qt::Orientation m_orientation;
    int m_dockHeight;
    QSize m_itemVisualSize;
    int m_itemSpacing;
    int m_itemPadding;
};

}
