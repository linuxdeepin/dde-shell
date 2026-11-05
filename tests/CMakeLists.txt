# SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: CC0-1.0

add_subdirectory(panels)
add_subdirectory(frame)
add_subdirectory(applets)

# ---- coverage: run all dde-shell module tests and generate coverage reports ----
# Trigger the per-subdir frame_coverage/applets_coverage/dock_coverage targets,
# each scoped to its module (ctest -R). Scoping keeps the pre-existing
# taskmanager/notification tests (which fail on master) out of the coverage run.
add_custom_target(coverage
    DEPENDS frame_coverage applets_coverage dock_coverage
    COMMENT "Run all dde-shell module tests and generate coverage reports"
)
