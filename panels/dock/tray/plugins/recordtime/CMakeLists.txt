# SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

set(PLUGIN_NAME "recordtime")

project(PLUGIN_NAME)

file(GLOB SRCS "*.h" "*.cpp")

find_package(PkgConfig REQUIRED)
find_package(Qt6Widgets REQUIRED)
find_package(Qt6Svg REQUIRED)
find_package(Qt6DBus REQUIRED)
find_package(Dtk6Widget REQUIRED)
find_package(Dtk6Gui REQUIRED)

pkg_check_modules(DFrameworkDBus REQUIRED dframeworkdbus)

add_definitions("${QT_DEFINITIONS} -DQT_PLUGIN")
add_library(${PLUGIN_NAME} SHARED ${SRCS} res.qrc)
set_target_properties(${PLUGIN_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ..)
target_include_directories(${PLUGIN_NAME} PUBLIC
    ${Dtk6Widget_INCLUDE_DIRS}
    ${Dtk6Gui_INCLUDE_DIRS}
    ${DFrameworkDBus_INCLUDE_DIRS}
    ../../interfaces
)
target_link_libraries(${PLUGIN_NAME} PRIVATE
    ${Qt6DBus_LIBRARIES}
      Dtk6::Widget
      Dtk6::Gui
    ${Qt6Widgets_LIBRARIES}
    ${Qt6Svg_LIBRARIES}
    ${DFrameworkDBus_LIBRARIES}
)

set(CMAKE_INSTALL_PREFIX "/usr")
install(TARGETS ${PLUGIN_NAME} LIBRARY DESTINATION lib/dde-dock/tmp/plugins)
