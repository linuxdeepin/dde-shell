# SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: CC0-1.0

find_package(Qt${QT_VERSION_MAJOR} ${REQUIRED_QT_VERSION} REQUIRED COMPONENTS Widgets)

add_executable(dde-shell
    main.cpp
)

target_link_libraries(dde-shell PRIVATE
    dde-shell-frame
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Dtk${DTK_VERSION_MAJOR}::Gui
)

install(TARGETS dde-shell DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

