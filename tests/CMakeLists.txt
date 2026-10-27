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

# QML lifecycle and folder component smoke tests. No desktop session or app
# manager is started; native window interactions are covered by manual testing.
find_program(QML_TEST_RUNNER NAMES qmltestrunner6 qmltestrunner
    HINTS /usr/lib/qt${QT_VERSION_MAJOR}/bin)
if(QML_TEST_RUNNER)
    foreach(_suite frame/qml)
        string(REPLACE "/" "_" _name "${_suite}")
        add_test(NAME ${_name}
            COMMAND ${CMAKE_COMMAND} -E env
                "QT_QPA_PLATFORM=offscreen" "QT_QUICK_BACKEND=software"
                "LD_PRELOAD=$<TARGET_FILE:dde-shell-frame>:$<TARGET_FILE:dde-shell-dock>"
                ${QML_TEST_RUNNER} -input "${CMAKE_CURRENT_SOURCE_DIR}/${_suite}"
                -import "${CMAKE_BINARY_DIR}/plugins")
    endforeach()
endif()
