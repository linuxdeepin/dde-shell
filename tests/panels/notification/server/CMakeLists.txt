# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(GTest REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} ${REQUIRED_QT_VERSION} REQUIRED COMPONENTS
    Core
    Test
    DBus
    Sql
)

add_executable(notifyserverapplet_tests
    ${CMAKE_SOURCE_DIR}/panels/notification/server/notifyserverapplet.h
    ${CMAKE_SOURCE_DIR}/panels/notification/server/notifyserverapplet.cpp
    ${CMAKE_SOURCE_DIR}/panels/notification/server/notificationmanager.h
    ${CMAKE_SOURCE_DIR}/panels/notification/server/notificationmanager.cpp
    ${CMAKE_SOURCE_DIR}/panels/notification/server/dbusadaptor.h
    ${CMAKE_SOURCE_DIR}/panels/notification/server/dbusadaptor.cpp
    ${CMAKE_SOURCE_DIR}/panels/notification/server/notificationsetting.h
    ${CMAKE_SOURCE_DIR}/panels/notification/server/notificationsetting.cpp

    ${CMAKE_SOURCE_DIR}/panels/notification/common/notifyentity.h
    ${CMAKE_SOURCE_DIR}/panels/notification/common/notifyentity.cpp
    ${CMAKE_SOURCE_DIR}/panels/notification/common/dataaccessor.h
    ${CMAKE_SOURCE_DIR}/panels/notification/common/dbaccessor.h
    ${CMAKE_SOURCE_DIR}/panels/notification/common/dbaccessor.cpp
    ${CMAKE_SOURCE_DIR}/panels/notification/common/memoryaccessor.h
    ${CMAKE_SOURCE_DIR}/panels/notification/common/memoryaccessor.cpp
    ${CMAKE_SOURCE_DIR}/panels/notification/common/notifysetting.h
    ${CMAKE_SOURCE_DIR}/panels/notification/common/notifysetting.cpp

    notifyserverapplet_test.cpp
)

target_compile_options(notifyserverapplet_tests PRIVATE
    -fvisibility=hidden
    -fvisibility-inlines-hidden
)

target_include_directories(notifyserverapplet_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/panels/notification/server
    ${CMAKE_SOURCE_DIR}/panels/notification/common
    ${CMAKE_SOURCE_DIR}/frame
)

target_link_libraries(notifyserverapplet_tests PRIVATE
    GTest::GTest
    GTest::gmock
    GTest::gmock_main
    GTest::Main

    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Test
    Qt${QT_VERSION_MAJOR}::DBus
    Qt${QT_VERSION_MAJOR}::Sql

    dde-shell-frame
    ds-notification-shared
)

add_test(
    NAME notifyserverapplet_tests
    COMMAND ${CMAKE_COMMAND} -E env
        LD_LIBRARY_PATH=${CMAKE_BINARY_DIR}/frame:${CMAKE_BINARY_DIR}/panels/notification
        $<TARGET_FILE:notifyserverapplet_tests>
)