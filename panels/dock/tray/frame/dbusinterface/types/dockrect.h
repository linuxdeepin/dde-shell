// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DOCKRECT_H
#define DOCKRECT_H

// #include <QRect>
#include <QDBusMetaType>

struct DockRect
{
public:
    DockRect();
    // operator QRect() const;

    friend QDebug operator<<(QDebug debug, const DockRect &rect);
    friend const QDBusArgument &operator>>(const QDBusArgument &arg, DockRect &rect);
    friend QDBusArgument &operator<<(QDBusArgument &arg, const DockRect &rect);

    friend constexpr inline bool operator==(const DockRect &r1, const DockRect &r2) noexcept
    { return r1.x==r2.x && r1.y==r2.y && r1.w==r2.w && r1.h==r2.h; }

    friend constexpr inline bool operator!=(const DockRect &r1, const DockRect &r2) noexcept
    { return !(r1 == r2); }


    int x;
    int y;
    uint w;
    uint h;
};

Q_DECLARE_METATYPE(DockRect)

void registerDockRectMetaType();

#endif // DOCKRECT_H
