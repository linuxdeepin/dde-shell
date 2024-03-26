// Copyright (C) 2011 ~ 2017 Deepin Technology Co., Ltd.
// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef SCREENRECT_H
#define SCREENRECT_H

// #include <QRect>
#include <QDBusArgument>
#include <QDebug>
#include <QDBusMetaType>

struct ScreenRect
{
public:
    ScreenRect();
    // operator QRect() const;

    friend QDebug operator<<(QDebug debug, const ScreenRect &rect);
    friend const QDBusArgument &operator>>(const QDBusArgument &arg, ScreenRect &rect);
    friend QDBusArgument &operator<<(QDBusArgument &arg, const ScreenRect &rect);

    friend constexpr inline bool operator==(const ScreenRect &r1, const ScreenRect &r2) noexcept
    { return r1.x==r2.x && r1.y==r2.y && r1.w==r2.w && r1.h==r2.h; }

    friend constexpr inline bool operator!=(const ScreenRect &r1, const ScreenRect &r2) noexcept
    { return !(r1 == r2); }

private:
    qint16 x;
    qint16 y;
    quint16 w;
    quint16 h;
};

Q_DECLARE_METATYPE(ScreenRect)

void registerScreenRectMetaType();

#endif // SCREENRECT_H
