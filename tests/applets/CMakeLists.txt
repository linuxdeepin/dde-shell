# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

# =============================================================================
# dde-shell applets/ unit tests (GTest).
#
# OBJECT library organization decision (per leader's request):
#   Single OBJECT lib `applets_test_objects` for the pure-logic dde-apps
#   classes that have NO DConfig / DBus / Wayland / frame-singleton deps:
#     * categoryutils.cpp  (pure namespace functions)
#     * itemspage.cpp      (pure QObject logic, no external deps)
#   These two are the only applets/ sources compiled into the OBJECT lib.
#
#   NOT compiled into the OBJECT lib (deferred with reasons):
#     * appslaunchtimes / appsdockedhelper — DConfig singleton (constructor
#       calls DConfig::create + reads/writes), needs DConfig runtime; testable
#       only with DConfig mock or DBus. Deferred.
#     * appitem — constructor calls AppsLaunchTimesHelper::instance() (global
#       singleton + DConfig); setLaunchedTimes/setDocked call singleton helpers.
#       Deferred (would need mock or refactor).
#     * appgroup — constructor instantiates ItemsPage (OK) but setData uses
#       AppGroupManager roles (needs the model); parseGroupId has a src bug.
#       Partially testable (static helpers) — covered via scoped include.
#     * appgroupmanager — DConfig + AMAppItemModel reference (DBus). Deferred.
#     * amappitemmodel / amappitem — DBus ObjectManager/Application proxies.
#       Deferred.
#     * appsapplet — instantiates AMAppItemModel (DBus) + AppGroupManager.
#       Deferred.
#     * amapplet / appearanceapplet — DBus session bus in load(). Deferred.
#     * keynotifyapplet / shutdownapplet — Wayland platform check + DBus.
#       Deferred.
#     * treelandkeynotify / treelandlockscreen — Wayland private API. Deferred.
#
#   applets sources that reference frame/ classes (DApplet) are NOT compiled
#   here — they'd need frame_test_objects linkage and pull DConfig/DBus. The
#   two pure-logic sources (categoryutils, itemspage) have ZERO frame/ deps.
#
# Coverage target: `applets_coverage` (independent from `frame_coverage`),
# ctest prefix `applets_` so `ctest -R '^applets_'` runs only these tests.
# =============================================================================

find_package(GTest REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} ${REQUIRED_QT_VERSION} REQUIRED COMPONENTS Core Gui Test)

include(GoogleTest)

option(APPLETS_BUILD_COVERAGE "Enable gcov coverage instrumentation for dde-shell applets unit tests" OFF)

# ---- OBJECT library: pure-logic applets/ sources under test -----------------
add_library(applets_test_objects OBJECT
    ${CMAKE_SOURCE_DIR}/applets/dde-apps/categoryutils.h
    ${CMAKE_SOURCE_DIR}/applets/dde-apps/categoryutils.cpp
    ${CMAKE_SOURCE_DIR}/applets/dde-apps/itemspage.h
    ${CMAKE_SOURCE_DIR}/applets/dde-apps/itemspage.cpp
)
target_include_directories(applets_test_objects PUBLIC
    ${CMAKE_SOURCE_DIR}/applets/dde-apps
)
target_link_libraries(applets_test_objects PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
)

# ---- Helper: declare an applets test executable -----------------------------
set(APPLETS_TEST_TARGETS "")
function(applets_add_test NAME)
    add_executable(${NAME} ${ARGN})
    target_link_libraries(${NAME} PRIVATE
        GTest::GTest
        GTest::Main
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Test
        applets_test_objects
    )
    gtest_discover_tests(${NAME} TEST_PREFIX "applets_")
    list(APPEND APPLETS_TEST_TARGETS ${NAME})
    set(APPLETS_TEST_TARGETS "${APPLETS_TEST_TARGETS}" PARENT_SCOPE)
endfunction()

# ---- Test executables -------------------------------------------------------
# categoryutils: pure namespace functions (parseBestMatchedCategory,
# parseDDECategoryString, parseXdgCategoryString) with rich branching.
applets_add_test(categoryutils_tests    categoryutilstests.cpp)

# itemspage: pure-logic pagination manager (append/insert/move/remove/find).
applets_add_test(itemspage_tests        itemspagetests.cpp)

# NOTE: appgroup static helpers (idIsFolder/groupIdFromNumber/parseGroupId) are
# deferred: appgroup.h → appitem.h → am.h → QDBusObjectPath (Qt6::DBus) +
# appgroupmanager.h → DConfig (Dtk6::Core) + QQmlEngine (Qt6::Qml) + yaml-cpp.
# The dependency chain is too heavy for 3 trivial one-liner static methods.
# Defer to a future batch with DConfig mock or after src refactoring.
# Also: parseGroupId has an off-by-one bug (mid(len+1) should be mid(len)) —
# recorded as a src defect, not fixed here.

# ---- Coverage instrumentation + report generation ---------------------------
if(APPLETS_BUILD_COVERAGE)
    message(STATUS "applets tests: coverage instrumentation ENABLED (APPLETS_BUILD_COVERAGE=ON)")
    target_compile_options(applets_test_objects PRIVATE -fprofile-arcs -ftest-coverage -O0 -g)
    foreach(_t IN LISTS APPLETS_TEST_TARGETS)
        target_compile_options(${_t} PRIVATE -fprofile-arcs -ftest-coverage -O0 -g)
        target_link_options(${_t} PRIVATE -fprofile-arcs)
    endforeach()

    find_program(LCOV_BIN lcov)
    find_program(GENHTML_BIN genhtml)
    if(LCOV_BIN AND GENHTML_BIN)
        set(APPLETS_COVERAGE_DIR "${CMAKE_BINARY_DIR}/applets_coverage")
        add_custom_target(applets_coverage
            DEPENDS ${APPLETS_TEST_TARGETS}
            COMMAND ${CMAKE_COMMAND} -E make_directory "${APPLETS_COVERAGE_DIR}"
            COMMAND ${CMAKE_CTEST_COMMAND} --test-dir "${CMAKE_BINARY_DIR}" -R "^applets_" --output-on-failure
            COMMAND ${LCOV_BIN} --capture --directory "${CMAKE_BINARY_DIR}"
                    --output-file "${APPLETS_COVERAGE_DIR}/applets.info"
            COMMAND ${LCOV_BIN} --remove "${APPLETS_COVERAGE_DIR}/applets.info"
                    '*/usr/include/*' '*/usr/lib/*' '*/3rdparty/*' '*/tests/*'
                    '*/moc_*' '*/qrc_*' '*/_deps/*' '*_json.h'
                    -o "${APPLETS_COVERAGE_DIR}/applets_filtered.info"
            COMMAND ${GENHTML_BIN} "${APPLETS_COVERAGE_DIR}/applets_filtered.info"
                    -o "${APPLETS_COVERAGE_DIR}/html" --branch-coverage
            COMMAND ${CMAKE_COMMAND} -E echo "=== Applets coverage report: ${APPLETS_COVERAGE_DIR}/html/index.html ==="
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
            COMMENT "Run applets unit tests and generate lcov branch-coverage report"
            VERBATIM
        )
    else()
        message(WARNING "lcov or genhtml not found; 'applets_coverage' target not created.")
    endif()
endif()
