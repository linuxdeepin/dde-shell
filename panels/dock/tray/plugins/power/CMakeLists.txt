
set(PLUGIN_NAME "power")

project(${PLUGIN_NAME})

generation_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/dbusinterface/xml ${CMAKE_CURRENT_SOURCE_DIR}/dbusinterface/generation_dbus_interface)

# Sources files
file(GLOB_RECURSE SRCS "*.h"
    "*.cpp"
    "../../widgets/tipswidget.h"
    "../../widgets/tipswidget.cpp"
    "../../frame/qtdbusextended/*.h"
    "../../frame/qtdbusextended/*.cpp")

find_package(PkgConfig REQUIRED)
find_package(Qt6Widgets REQUIRED)
find_package(Qt6Svg REQUIRED)
find_package(Qt6DBus REQUIRED)
find_package(Dtk6Widget REQUIRED)
find_package(Dtk6Gui REQUIRED)

add_definitions("${QT_DEFINITIONS} -DQT_PLUGIN")
add_library(${PLUGIN_NAME} SHARED ${SRCS} power.qrc)
set_target_properties(${PLUGIN_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ../quick-trays)
target_include_directories(${PLUGIN_NAME} PUBLIC ${DtkWidget_INCLUDE_DIRS}
                                                 ../../interfaces
                                                 ../../frame/qtdbusextended
                                                 ./dbusinterface/generation_dbus_interface)

target_link_libraries(${PLUGIN_NAME} PRIVATE
    ${Dtk6Widget_LIBRARIES}
    Qt6::Widgets
    Dtk6::Gui
    Dtk6::Widget
    Qt6::DBus
    Qt6::Svg)

install(TARGETS ${PLUGIN_NAME} LIBRARY DESTINATION lib/dde-dock/tmp/plugins/quick-trays)
