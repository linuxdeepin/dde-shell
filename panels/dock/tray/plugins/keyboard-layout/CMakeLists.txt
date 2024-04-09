
set(PLUGIN_NAME "keyboard-layout")

project(${PLUGIN_NAME})

generation_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/dbusinterface/xml ${CMAKE_CURRENT_SOURCE_DIR}/dbusinterface/generation_dbus_interface)

# Sources files
file(GLOB_RECURSE SRCS "*.h"
    "*.cpp"
    "../../frame/qtdbusextended/*.h"
    "../../frame/qtdbusextended/*.cpp")

find_package(PkgConfig REQUIRED)
find_package(Qt6Widgets REQUIRED)
find_package(Qt6Svg REQUIRED)
find_package(Dtk6Widget REQUIRED)
find_package(Dtk6Gui REQUIRED)
find_package(Qt6DBus REQUIRED)

add_definitions("${QT_DEFINITIONS} -DQT_PLUGIN")
add_library(${PLUGIN_NAME} SHARED ${SRCS})
set_target_properties(${PLUGIN_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ../)
target_include_directories(${PLUGIN_NAME} PUBLIC ${DtkWidget_INCLUDE_DIRS}
                                                 ../../interfaces
                                                 ./dbusinterface
                                                 ./dbusinterface/generation_dbus_interface
                                                 ../../frame/util
                                                 ../../frame/qtdbusextended)

target_link_libraries(${PLUGIN_NAME} PRIVATE
    ${Dtk6Widget_LIBRARIES}
    Qt6::Widgets
    Qt6::DBus
    Dtk6::Widget
    Dtk6::Gui
    Qt6::Svg)

install(TARGETS ${PLUGIN_NAME} LIBRARY DESTINATION lib/dde-dock/tmp/plugins/)
install(FILES ./keybord_layout.json DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/dde-dock/indicator)
