// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dsutility.h"

#include <dobject_p.h>

#ifdef BUILD_WITH_X11
#include "private/utility_x11_p.h"
#endif

DS_BEGIN_NAMESPACE
DCORE_USE_NAMESPACE

class UtilityPrivate : public DObjectPrivate
{
public:
    explicit UtilityPrivate(Utility *qq)
        : DObjectPrivate(qq)
    {

    }
    ~UtilityPrivate() override
    {

    }

    D_DECLARE_PUBLIC(Utility);
};

Utility *Utility::instance()
{
    static Utility* g_instance = nullptr;
    if (g_instance == nullptr) {
#ifdef BUILD_WITH_X11
        g_instance = new X11Utility();
#else
        g_instance = new Utility();
#endif
    }
    return g_instance;
}

Utility::Utility(QObject *parent)
    : QObject(parent)
    , DObject(*new UtilityPrivate(this), this)
{

}

bool Utility::grabKeyboard(QWindow *target, bool grab)
{
    Q_UNUSED(target)
    Q_UNUSED(grab)
    return false;
}

bool Utility::grabMouse(QWindow *target, bool grab)
{
    Q_UNUSED(target)
    Q_UNUSED(grab)
    return false;
}

DS_END_NAMESPACE
