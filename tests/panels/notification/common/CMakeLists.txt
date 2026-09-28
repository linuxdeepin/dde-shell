# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(GTest REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} ${REQUIRED_QT_VERSION} REQUIRED COMPONENTS
    Core
    Test
)

add_executable(expiretimer_tests
    ${CMAKE_SOURCE_DIR}/panels/notification/common/expiretimer.h
    ${CMAKE_SOURCE_DIR}/panels/notification/common/expiretimer.cpp
    ${CMAKE_SOURCE_DIR}/panels/notification/common/notifyentity.h
    ${CMAKE_SOURCE_DIR}/panels/notification/common/notifyentity.cpp

    expiretimer_test.cpp
)

target_compile_options(expiretimer_tests PRIVATE
    -fvisibility=hidden
    -fvisibility-inlines-hidden
)

target_include_directories(expiretimer_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/panels/notification/common
)

target_link_libraries(expiretimer_tests PRIVATE
    GTest::GTest
    GTest::gmock
    GTest::gmock_main
    GTest::Main

    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Test

    ds-notification-shared
)

add_test(
    NAME expiretimer_tests
    COMMAND ${CMAKE_COMMAND} -E env
        LD_LIBRARY_PATH=${CMAKE_BINARY_DIR}/frame:${CMAKE_BINARY_DIR}/panels/notification
        $<TARGET_FILE:expiretimer_tests>
)
