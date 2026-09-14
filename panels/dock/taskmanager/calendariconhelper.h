// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>

namespace dock {

// Generate a dynamic calendar SVG icon with the current date to the given file path.
// Returns true on success, false if the file could not be created.
// Ported from dde-launchpad's IconUtils::createCalendarIcon().
bool createCalendarIcon(const QString &fileName);

} // namespace dock
