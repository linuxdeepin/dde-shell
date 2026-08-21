# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

# Dock panels unit tests (taskmanager hoverpreviewproxymodel + globals,
# frame dockiteminfo, appruntimeitem windowmanager, tray trayitempositionmanager).
#
# Independent OBJECT library `dock_test_objects` (only for non-Q_OBJECT source)
# to avoid symbol clashes with frame_test_objects / applets_test_objects
# (esp. Q_LOGGING_CATEGORY) and to avoid AUTOMOC moc_*.cpp duplicate-symbol
# conflicts. Q_OBJECT/Q_GADGET production sources are compiled directly into
# their own test executable (same pattern as the existing taskmanager tests),
# because compiling a Q_OBJECT source in both an OBJECT library and a test
# executable that links its objects would produce duplicate moc symbols.
#
# Existing tests/panels/dock/taskmanager tests are untouched.

find_package(GTest REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} ${REQUIRED_QT_VERSION} REQUIRED COMPONENTS
    Core
    Gui
    DBus
    Test
)

include(GoogleTest)

option(DOCK_BUILD_COVERAGE "Enable gcov coverage instrumentation for dock tests" OFF)

# Common helper: coverage flags for a target (real sources only, not moc).
function(dock_apply_coverage target)
    if(DOCK_BUILD_COVERAGE)
        target_compile_options(${target} PRIVATE -fprofile-arcs -ftest-coverage -O0 -g)
        target_link_options(${target} PRIVATE -fprofile-arcs -ftest-coverage)
    endif()
endfunction()

# Common helper: expose protected/private members as public in the test build.
function(dock_expose_visibility target)
    target_compile_definitions(${target} PRIVATE protected=public private=public)
endfunction()

# ---------------------------------------------------------------------------
# Test source objects — only the non-Q_OBJECT source (dockiteminfo.cpp) is
# shared via an OBJECT library. Q_OBJECT/Q_GADGET sources are compiled directly
# into their own test executable to avoid AUTOMOC moc duplicate-symbol clashes.
# ---------------------------------------------------------------------------
add_library(dock_test_objects OBJECT
    ${CMAKE_SOURCE_DIR}/panels/dock/frame/dockiteminfo.cpp
)

target_include_directories(dock_test_objects PRIVATE
    ${CMAKE_SOURCE_DIR}/panels/dock/frame
)

target_link_libraries(dock_test_objects PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::DBus
)

dock_expose_visibility(dock_test_objects)
dock_apply_coverage(dock_test_objects)

# ---------------------------------------------------------------------------
# HoverPreviewProxyModel tests (taskmanager)
# ---------------------------------------------------------------------------
add_executable(hoverpreviewproxymodel_tests
    ${CMAKE_SOURCE_DIR}/panels/dock/taskmanager/hoverpreviewproxymodel.h
    ${CMAKE_SOURCE_DIR}/panels/dock/taskmanager/hoverpreviewproxymodel.cpp
    hoverpreviewproxymodeltests.cpp
    sourcemodel.h
)

target_include_directories(hoverpreviewproxymodel_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs
    ${CMAKE_SOURCE_DIR}/panels/dock/taskmanager
)

target_link_libraries(hoverpreviewproxymodel_tests PRIVATE
    GTest::GTest
    GTest::Main
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Test
)

dock_expose_visibility(hoverpreviewproxymodel_tests)
dock_apply_coverage(hoverpreviewproxymodel_tests)
gtest_discover_tests(hoverpreviewproxymodel_tests)

# ---------------------------------------------------------------------------
# globals.h inline function tests (escapeToObjectPath / unescapeFromObjectPath)
# ---------------------------------------------------------------------------
add_executable(globalstests
    globalstests.cpp
)

target_include_directories(globalstests PRIVATE
    ${CMAKE_SOURCE_DIR}/panels/dock/taskmanager
)

target_link_libraries(globalstests PRIVATE
    GTest::GTest
    GTest::Main
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Test
)

dock_apply_coverage(globalstests)
gtest_discover_tests(globalstests)

# ---------------------------------------------------------------------------
# DockItemInfo tests (frame) — uses the shared OBJECT library.
# ---------------------------------------------------------------------------
add_executable(dockiteminfo_tests
    dockiteminfotests.cpp
    $<TARGET_OBJECTS:dock_test_objects>
)

target_include_directories(dockiteminfo_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/panels/dock/frame
)

target_link_libraries(dockiteminfo_tests PRIVATE
    GTest::GTest
    GTest::Main
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::DBus
    Qt${QT_VERSION_MAJOR}::Test
)

dock_expose_visibility(dockiteminfo_tests)
dock_apply_coverage(dockiteminfo_tests)
gtest_discover_tests(dockiteminfo_tests)

# ---------------------------------------------------------------------------
# WindowManager tests (appruntimeitem)
# ---------------------------------------------------------------------------
add_executable(windowmanager_tests
    ${CMAKE_SOURCE_DIR}/panels/dock/appruntimeitem/windowmanager.h
    ${CMAKE_SOURCE_DIR}/panels/dock/appruntimeitem/windowmanager.cpp
    windowmanagertests.cpp
)

target_include_directories(windowmanager_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/panels/dock/appruntimeitem
)

target_link_libraries(windowmanager_tests PRIVATE
    GTest::GTest
    GTest::Main
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Test
)

dock_expose_visibility(windowmanager_tests)
dock_apply_coverage(windowmanager_tests)
gtest_discover_tests(windowmanager_tests)

# ---------------------------------------------------------------------------
# TrayItemPositionManager tests (tray)
# Qt6 Qml dev package is not installed, so a stub <QQmlEngine> header
# (defining empty QML_ELEMENT/QML_SINGLETON macros and forward-declaring
# QQmlEngine/QJSEngine) is placed first in the include path.
# ---------------------------------------------------------------------------
add_executable(trayitempositionmanager_tests
    ${CMAKE_SOURCE_DIR}/panels/dock/tray/trayitempositionmanager.h
    ${CMAKE_SOURCE_DIR}/panels/dock/tray/trayitempositionmanager.cpp
    trayitempositionmanagertests.cpp
)

target_include_directories(trayitempositionmanager_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs
    ${CMAKE_SOURCE_DIR}/panels/dock/tray
)

target_link_libraries(trayitempositionmanager_tests PRIVATE
    GTest::GTest
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Test
)

dock_expose_visibility(trayitempositionmanager_tests)
dock_apply_coverage(trayitempositionmanager_tests)
gtest_discover_tests(trayitempositionmanager_tests)

# ---------------------------------------------------------------------------
# Coverage report target (output to build dir, generated code filtered out).
# Generated moc/qrc/ui/qml code is not instrumented (AUTOMOC only runs on the
# test executables; the non-Q_OBJECT OBJECT lib produces no moc), and lcov
# filters any residual generated code as a fallback.
# ---------------------------------------------------------------------------
if(DOCK_BUILD_COVERAGE)
    find_program(LCOV_BIN lcov)
    find_program(GENHTML_BIN genhtml)
    set(DOCK_COVERAGE_DIR ${CMAKE_BINARY_DIR}/coverage_report/dock)
    set(DOCK_COVERAGE_INFO ${DOCK_COVERAGE_DIR}/dock_coverage.info)
    set(DOCK_COVERAGE_FILTERED ${DOCK_COVERAGE_DIR}/dock_coverage_filtered.info)

    add_custom_target(dock_coverage
        COMMAND ${CMAKE_COMMAND} -E make_directory ${DOCK_COVERAGE_DIR}
        COMMAND ${LCOV_BIN} --capture --directory ${CMAKE_BINARY_DIR}
                --output-file ${DOCK_COVERAGE_INFO}
                --rc lcov_branch_coverage=1
        COMMAND ${LCOV_BIN} --remove ${DOCK_COVERAGE_INFO}
                '*/moc_*.cpp' '*/moc_*.h'
                '*/qrc_*.cpp' '*/qrc_*.h'
                '*/ui_*.h'
                '*/qml_*.h' '*/qmlcache_*.cpp'
                '*/_autogen/*'
                '*_json.h'
                '*/3rdparty/*'
                '*/generated/*'
                '/usr/*'
                '*/tests/*'
                --output-file ${DOCK_COVERAGE_FILTERED}
                --rc lcov_branch_coverage=1
        COMMAND ${GENHTML_BIN} ${DOCK_COVERAGE_FILTERED}
                --output-directory ${DOCK_COVERAGE_DIR}/html
                --rc lcov_branch_coverage=1
        DEPENDS
            hoverpreviewproxymodel_tests
            globalstests
            dockiteminfo_tests
            windowmanager_tests
            trayitempositionmanager_tests
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generating dock coverage report in ${DOCK_COVERAGE_DIR}"
    )
endif()
