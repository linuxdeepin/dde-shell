// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "mirrormodepolicy.h"

#include <gtest/gtest.h>

using namespace dock;

namespace {

// The values kwin's Workspace::updateMirrorModeProperty() writes.
TEST(MirrorModePolicy, mapsRootPropertyValue)
{
    EXPECT_EQ(mirrorModePolicyFromRootProperty(0), MirrorModePolicy::Off);
    EXPECT_EQ(mirrorModePolicyFromRootProperty(1), MirrorModePolicy::Blacklist);
    EXPECT_EQ(mirrorModePolicyFromRootProperty(2), MirrorModePolicy::Whitelist);
    EXPECT_EQ(mirrorModePolicyFromRootProperty(3), MirrorModePolicy::Passthrough);
    // A layout this build does not know must not grow a control.
    EXPECT_EQ(mirrorModePolicyFromRootProperty(4), MirrorModePolicy::Off);
    EXPECT_EQ(mirrorModePolicyFromRootProperty(0xffffffffu), MirrorModePolicy::Off);
}

TEST(MirrorModePolicy, approvesOnlyWhereAWindowCanBeChosen)
{
    EXPECT_FALSE(mirrorModeCanApprove(MirrorModePolicy::Off));
    EXPECT_FALSE(mirrorModeCanApprove(MirrorModePolicy::Passthrough));
    EXPECT_TRUE(mirrorModeCanApprove(MirrorModePolicy::Blacklist));
    EXPECT_TRUE(mirrorModeCanApprove(MirrorModePolicy::Whitelist));
}

// Mirrors kwin's Workspace::mirrorModeShouldMirror().
TEST(MirrorModePolicy, mirrorsPerPolicy)
{
    EXPECT_FALSE(mirrorModeIsApproved(MirrorModePolicy::Off, false));
    EXPECT_FALSE(mirrorModeIsApproved(MirrorModePolicy::Off, true));

    EXPECT_TRUE(mirrorModeIsApproved(MirrorModePolicy::Passthrough, false));
    EXPECT_TRUE(mirrorModeIsApproved(MirrorModePolicy::Passthrough, true));

    EXPECT_TRUE(mirrorModeIsApproved(MirrorModePolicy::Blacklist, false));
    EXPECT_FALSE(mirrorModeIsApproved(MirrorModePolicy::Blacklist, true));

    EXPECT_FALSE(mirrorModeIsApproved(MirrorModePolicy::Whitelist, false));
    EXPECT_TRUE(mirrorModeIsApproved(MirrorModePolicy::Whitelist, true));
}

// Toggling twice has to land back where it started, in both listable policies.
TEST(MirrorModePolicy, listedFlagRoundTripsWithApproval)
{
    for (auto policy : { MirrorModePolicy::Blacklist, MirrorModePolicy::Whitelist }) {
        for (bool approved : { false, true }) {
            const bool listed = mirrorModeListedForApproval(policy, approved);
            EXPECT_EQ(mirrorModeIsApproved(policy, listed), approved);

            const bool toggled = mirrorModeListedForApproval(policy, !approved);
            EXPECT_EQ(mirrorModeIsApproved(policy, toggled), !approved);
        }
    }
}

} // namespace
