// Copyright (C) 2011 ~ 2018 Deepin Technology Co., Ltd.
// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "dsglobal.h"

#include <QtCore>

DS_BEGIN_NAMESPACE
namespace dock {

constexpr uint MIN_DOCK_SIZE = 40;
constexpr uint MAX_DOCK_SIZE = 100;

///
/// \brief The DisplayMode enum
/// spec dock display mode
///
enum DisplayMode {
    Fashion     = 0,
    Efficient   = 1,
    // deprecreated
//    Classic     = 2,
};

///
/// \brief The HideMode enum
/// spec dock hide behavior
///
enum HideMode {
    KeepShowing     = 0,
    KeepHidden      = 1,
    SmartHide       = 2
};

enum Position {
    Top     = 0,
    Right   = 1,
    Bottom  = 2,
    Left    = 3
};

///
/// \brief The HideState enum
/// spec current dock should hide or shown.
/// this argument works only HideMode is SmartHide
///
enum HideState {
    Unknown     = 0,
    Show        = 1,
    Hide        = 2,
};

enum class AniAction {
    Show = 0,
    Hide
};
}

DS_END_NAMESPACE

Q_DECLARE_METATYPE(DS_NAMESPACE::dock::DisplayMode)
Q_DECLARE_METATYPE(DS_NAMESPACE::dock::Position)
