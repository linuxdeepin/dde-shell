# SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: CC0-1.0

# =============================================================================
# dde-shell frame/ core framework unit tests (GTest).
#
# New-workflow adaptation (DDE-107 round 3):
#   * Sources under test are compiled ONCE into a single OBJECT library
#     (frame_test_objects) and linked into every test executable, instead of
#     being recompiled per add_executable (round 1/2).  This keeps gcov
#     instrumentation in one place and yields a single dsLog definition
#     (Q_LOGGING_CATEGORY lives in pluginmetadata.cpp).
#   * protected/private members are exposed to test code via a scoped
#     `#define private public` block in listtotableproxymodeltests.cpp
#     (around the model header only), NOT via a global -D compile definition
#     (the latter breaks gtest/libstdc++ — see B1 review finding). Required
#     by listtotableproxymodeltests, which populates
#     ListToTableProxyModel::m_roles directly because the `roles` Q_PROPERTY is
#     only ever set from QML in production and QList<int> has no
#     Q_DECLARE_METATYPE in this repo.  kextracolumns and pluginfactory tests
#     use only public API and need no visibility macro.
#   * Coverage instrumentation + report generation are build-system targets:
#     option FRAME_BUILD_COVERAGE + custom target `frame_coverage`, emitting
#     into ${CMAKE_BINARY_DIR}/frame_coverage.
#   * Pattern (GTest + gtest_discover_tests, BUILD_TESTING gate) follows
#     tests/panels/dock/taskmanager.  Every TEST is registered with the
#     `frame_` ctest prefix so `ctest -R '^frame_'` runs only these tests.
# =============================================================================

find_package(GTest REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} ${REQUIRED_QT_VERSION} REQUIRED COMPONENTS Core Gui Qml Test)

include(GoogleTest)

option(FRAME_BUILD_COVERAGE "Enable gcov coverage instrumentation for dde-shell frame unit tests" OFF)

# ---- OBJECT library: all frame/ sources under test, compiled once ------------
# DS_LIB mirrors frame/CMakeLists.txt so the DS_SHARE export macro is correct
# while compiling these sources into the test objects.
add_library(frame_test_objects OBJECT
    ${CMAKE_SOURCE_DIR}/frame/pluginmetadata.h
    ${CMAKE_SOURCE_DIR}/frame/pluginmetadata.cpp
    ${CMAKE_SOURCE_DIR}/frame/appletdata.h
    ${CMAKE_SOURCE_DIR}/frame/appletdata.cpp
    ${CMAKE_SOURCE_DIR}/frame/appletitemmodel.h
    ${CMAKE_SOURCE_DIR}/frame/appletitemmodel.cpp
    ${CMAKE_SOURCE_DIR}/frame/models/kextracolumnsproxymodel.h
    ${CMAKE_SOURCE_DIR}/frame/models/kextracolumnsproxymodel.cpp
    ${CMAKE_SOURCE_DIR}/frame/models/listtotableproxymodel.h
    ${CMAKE_SOURCE_DIR}/frame/models/listtotableproxymodel.cpp
    ${CMAKE_SOURCE_DIR}/frame/pluginfactory.h
    ${CMAKE_SOURCE_DIR}/frame/pluginfactory.cpp
)
target_compile_definitions(frame_test_objects PRIVATE DS_LIB)
target_include_directories(frame_test_objects PUBLIC
    ${CMAKE_SOURCE_DIR}/frame
    ${CMAKE_SOURCE_DIR}/frame/models
)
target_link_libraries(frame_test_objects PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Qml
)

# ---- Helper: declare a frame test executable with the standard link set -------
set(FRAME_TEST_TARGETS "")
function(frame_add_test NAME)
    add_executable(${NAME} ${ARGN})
    target_link_libraries(${NAME} PRIVATE
        GTest::GTest
        GTest::Main
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Test
        frame_test_objects
    )
    gtest_discover_tests(${NAME} TEST_PREFIX "frame_")
    list(APPEND FRAME_TEST_TARGETS ${NAME})
    set(FRAME_TEST_TARGETS "${FRAME_TEST_TARGETS}" PARENT_SCOPE)
endfunction()

# ---- Test executables --------------------------------------------------------
# Pre-existing (round 1/2, reviewed & 45/45 passing) — test sources unchanged.
frame_add_test(pluginmetadata_tests   pluginmetadatatests.cpp)
frame_add_test(appletdata_tests       appletdatatests.cpp)
frame_add_test(appletitemmodel_tests  appletitemmodeltests.cpp)

# New (round 3) — extend coverage to previously-untested frame/ modules.
frame_add_test(proxymodel_tests
    kextracolumnsproxymodeltests.cpp
    listtotableproxymodeltests.cpp
)
frame_add_test(pluginfactory_tests    pluginfactorytests.cpp)

# Visibility: private-member access for ListToTableProxyModel::m_roles /
# m_sourceColumn is done via a scoped `#define private public` block in
# listtotableproxymodeltests.cpp itself (around the model header only), NOT
# via a global -Dprivate=public compile definition — the latter breaks
# gtest/libstdc++ (<sstream>/<any> access redeclaration). kextracolumns and
# pluginfactory tests use only public API and need no visibility macro.

# ---- Coverage instrumentation + report generation (build-system target) -------
if(FRAME_BUILD_COVERAGE)
    message(STATUS "frame tests: coverage instrumentation ENABLED (FRAME_BUILD_COVERAGE=ON)")
    # Instrument the single OBJECT lib (covers all sources under test) and the
    # test executables (so the link step pulls in libgcov).
    target_compile_options(frame_test_objects PRIVATE -fprofile-arcs -ftest-coverage -O0 -g)
    foreach(_t IN LISTS FRAME_TEST_TARGETS)
        target_compile_options(${_t} PRIVATE -fprofile-arcs -ftest-coverage -O0 -g)
        target_link_options(${_t} PRIVATE -fprofile-arcs)
    endforeach()

    find_program(LCOV_BIN lcov)
    find_program(GENHTML_BIN genhtml)
    if(LCOV_BIN AND GENHTML_BIN)
        set(FRAME_COVERAGE_DIR "${CMAKE_BINARY_DIR}/frame_coverage")
        add_custom_target(frame_coverage
            DEPENDS ${FRAME_TEST_TARGETS}
            COMMAND ${CMAKE_COMMAND} -E make_directory "${FRAME_COVERAGE_DIR}"
            COMMAND ${CMAKE_CTEST_COMMAND} --test-dir "${CMAKE_BINARY_DIR}" -R "^frame_" --output-on-failure
            COMMAND ${LCOV_BIN} --capture --directory "${CMAKE_BINARY_DIR}"
                    --output-file "${FRAME_COVERAGE_DIR}/frame.info"
            COMMAND ${LCOV_BIN} --remove "${FRAME_COVERAGE_DIR}/frame.info"
                    '*/usr/include/*' '*/usr/lib/*' '*/3rdparty/*' '*/tests/*'
                    '*/moc_*' '*/qrc_*' '*/_deps/*'
                    -o "${FRAME_COVERAGE_DIR}/frame_filtered.info"
            COMMAND ${GENHTML_BIN} "${FRAME_COVERAGE_DIR}/frame_filtered.info"
                    -o "${FRAME_COVERAGE_DIR}/html" --branch-coverage
            COMMAND ${CMAKE_COMMAND} -E echo "=== Frame coverage report: ${FRAME_COVERAGE_DIR}/html/index.html ==="
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
            COMMENT "Run frame unit tests and generate lcov branch-coverage report"
            VERBATIM
        )
    else()
        message(WARNING "lcov or genhtml not found; 'frame_coverage' target not created. Install the 'lcov' package to enable.")
    endif()
endif()
