# SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

add_library(notification-shared OBJECT
    ${CMAKE_SOURCE_DIR}/panels/notification/common/notifyentity.h
    ${CMAKE_SOURCE_DIR}/panels/notification/common/notifyentity.cpp
    ${CMAKE_SOURCE_DIR}/panels/notification/common/dbaccessor.h
    ${CMAKE_SOURCE_DIR}/panels/notification/common/dbaccessor.cpp
)

target_include_directories(notification-shared PUBLIC
    ${CMAKE_SOURCE_DIR}/panels/notification/
)
target_link_libraries(notification-shared PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Sql
)

add_subdirectory(bubble)
add_subdirectory(center)
add_subdirectory(osd)
add_subdirectory(server)
