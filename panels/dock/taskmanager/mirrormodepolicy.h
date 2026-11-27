// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>

namespace dock {

// The mirror mode switch and policy, as KWin publishes them in the
// _DEEPIN_NET_MIRROR_MODE root window property: 0 while the mode is off,
// otherwise the policy KWin applies to the windows it has not listed.
// See kwin's Workspace::updateMirrorModeProperty().
enum class MirrorModePolicy : uint32_t {
    Off = 0,
    Blacklist = 1,
    Whitelist = 2,
    Passthrough = 3,
};

inline MirrorModePolicy mirrorModePolicyFromRootProperty(uint32_t value)
{
    switch (value) {
    case 0:
        return MirrorModePolicy::Off;
    case 1:
        return MirrorModePolicy::Blacklist;
    case 2:
        return MirrorModePolicy::Whitelist;
    case 3:
        return MirrorModePolicy::Passthrough;
    default:
        // Unknown layout from a newer KWin: leave the control out of the way.
        return MirrorModePolicy::Off;
    }
}

// Only these two policies let a single window be approved by the user.
inline bool mirrorModeCanApprove(MirrorModePolicy policy)
{
    return policy == MirrorModePolicy::Blacklist || policy == MirrorModePolicy::Whitelist;
}

// Whether a window shows on the mirror outputs, given its listed flag.
// Mirrors kwin's Workspace::mirrorModeShouldMirror().
inline bool mirrorModeIsApproved(MirrorModePolicy policy, bool listed)
{
    switch (policy) {
    case MirrorModePolicy::Whitelist:
        return listed;
    case MirrorModePolicy::Blacklist:
        return !listed;
    case MirrorModePolicy::Passthrough:
        // Every window is mirrored whatever its flag says.
        return true;
    case MirrorModePolicy::Off:
        // The property reports a policy only while the mode is on.
        return false;
    }
    return false;
}

// The listed flag to write so that the approval state becomes `approved`.
inline bool mirrorModeListedForApproval(MirrorModePolicy policy, bool approved)
{
    return policy == MirrorModePolicy::Blacklist ? !approved : approved;
}

} // namespace dock
