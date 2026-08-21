# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later
set(DOCK_PUBLIC_HEADERS
    dockiteminfo.h
    dappletdock.h
)

add_library(dde-shell-dock SHARED
    ${DOCK_PUBLIC_HEADERS}
    dockiteminfo.cpp
    dappletdock.cpp
)

set_target_properties(dde-shell-dock PROPERTIES
    PUBLIC_HEADER "${DOCK_PUBLIC_HEADERS}"
    VERSION ${CMAKE_PROJECT_VERSION}
    SOVERSION 1
    OUTPUT_NAME dde-shell-dock
    EXPORT_NAME ShellDock
)

target_link_libraries(dde-shell-dock
PUBLIC
    dde-shell-frame
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Quick
)

target_include_directories(dde-shell-dock INTERFACE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
    $<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}>
)
target_link_directories(dde-shell-dock INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<INSTALL_INTERFACE:${LIB_INSTALL_DIR}>
)

target_compile_definitions(dde-shell-dock PRIVATE DS_LIB)

install(TARGETS dde-shell-dock EXPORT DDEShellDockTargets DESTINATION "${LIB_INSTALL_DIR}" PUBLIC_HEADER DESTINATION "${INCLUDE_INSTALL_DIR}/dock")

set(DOCK_CONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/DDEShellDock")

install(EXPORT DDEShellDockTargets NAMESPACE Dde:: FILE DDEShellDockTargets.cmake DESTINATION "${DOCK_CONFIG_INSTALL_DIR}")

include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${CMAKE_CURRENT_LIST_DIR}/DDEShellDockConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/DDEShellDockConfig.cmake"
    INSTALL_DESTINATION "${DOCK_CONFIG_INSTALL_DIR}"
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/DDEShellDockConfigVersion.cmake"
    VERSION ${CMAKE_PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/DDEShellDockConfig.cmake" DESTINATION "${DOCK_CONFIG_INSTALL_DIR}")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/DDEShellDockConfigVersion.cmake" DESTINATION "${DOCK_CONFIG_INSTALL_DIR}")
