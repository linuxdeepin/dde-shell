
set(PLUGIN_NAME "uos-ai-tray")

project(${PLUGIN_NAME})

# Sources files
file(GLOB_RECURSE SRCS "*.h" "*.cpp")

find_package(PkgConfig REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Svg DBus Widgets)
find_package(Dtk6Widget REQUIRED)
find_package(Dtk6Gui REQUIRED)

set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/dbus/org.desktopspec.ObjectManager1.xml
    PROPERTIES  INCLUDE dbus/types/am.h
                CLASSNAME ObjectManager
)

qt_add_dbus_interfaces(
    DBUS_INTERFACES
    ${CMAKE_CURRENT_SOURCE_DIR}/dbus/org.desktopspec.ObjectManager1.xml
)

add_definitions("${QT_DEFINITIONS} -DQT_PLUGIN")
add_library(${PLUGIN_NAME} SHARED ${SRCS} ${DBUS_INTERFACES} plugin-tray.qrc)
set_target_properties(${PLUGIN_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ../quick-trays)
target_include_directories(${PLUGIN_NAME} PUBLIC ${DtkWidget_INCLUDE_DIRS}
    ../../interfaces
)

target_link_libraries(${PLUGIN_NAME} PRIVATE
    ${Dtk6Widget_LIBRARIES}
    Qt6::Widgets
    Dtk6::Gui
    Dtk6::Widget
    Qt6::DBus
    Qt6::Svg)

install(TARGETS ${PLUGIN_NAME} LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}/plugins/quick-trays)
