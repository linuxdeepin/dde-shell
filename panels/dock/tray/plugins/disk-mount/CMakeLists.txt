cmake_minimum_required(VERSION 3.10)

set(PLUGIN_NAME dde-disk-mount-plugin)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 COMPONENTS Core Widgets DBus Concurrent REQUIRED)
find_package(Dtk6 COMPONENTS Widget Gui REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_search_module(LibMount REQUIRED mount IMPORTED_TARGET)

set(QRC_FILE
    resources.qrc
    resources/theme-icons.qrc
)
qt_add_resources(QRC_RESOURCES ${QRC_FILE})

qt_add_dbus_interface(MOUNT_PLUGIN_FILES
    "org.deepin.filemanager.server.DeviceManager.xml"
    devicemanager_interface
)

qt_add_library(${PLUGIN_NAME}
SHARED
    ${QRC_RESOURCES}
    disk-mount.json
)

target_sources(${PLUGIN_NAME}
PRIVATE
    device/typedefines.h
    device/dockitemdatamanager.cpp
    device/dockitemdatamanager.h
    utils/dockutils.cpp
    utils/dockutils.h
    widgets/deviceitem.cpp
    widgets/deviceitem.h
    widgets/devicelist.cpp
    widgets/devicelist.h
    widgets/diskpluginitem.cpp
    widgets/diskpluginitem.h
    widgets/tipswidget.cpp
    widgets/tipswidget.h
    diskmountplugin.cpp
    diskmountplugin.h
    ${MOUNT_PLUGIN_FILES}
)

set_target_properties(${PLUGIN_NAME}
PROPERTIES
LIBRARY_OUTPUT_DIRECTORY
    ../quick-trays
)

target_include_directories(${PLUGIN_NAME}
PUBLIC
    device
    utils
    widgets
    ../../interfaces # old dock interface
)

target_link_libraries(${PLUGIN_NAME}
PRIVATE
    Qt6::DBus
    Qt6::Concurrent
    Dtk6::Widget
    PkgConfig::LibMount
)

install(
TARGETS ${PLUGIN_NAME}
LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}/plugins/quick-trays
)
