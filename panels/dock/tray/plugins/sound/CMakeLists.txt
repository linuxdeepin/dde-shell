
set(PLUGIN_NAME "sound")

project(${PLUGIN_NAME})

generation_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/dbusinterface/xml ${CMAKE_CURRENT_SOURCE_DIR}/dbusinterface/generation_dbus_interface)

# Sources files
file(GLOB_RECURSE SRCS "*.h"
    "*.cpp"
    "../../widgets/*.h"
    "../../widgets/*.cpp"
    "../../frame/util/imageutil.h"
    "../../frame/util/imageutil.cpp"
    "../../frame/util/horizontalseperator.h"
    "../../frame/util/horizontalseperator.cpp"
    "../../frame/qtdbusextended/*.h"
    "../../frame/qtdbusextended/*.cpp")

find_package(PkgConfig REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Concurrent DBus Svg Widgets)
find_package(Dtk6Widget REQUIRED)
find_package(Dtk6Gui REQUIRED)

pkg_check_modules(XCB_LIBS REQUIRED IMPORTED_TARGET xcursor)

add_definitions("${QT_DEFINITIONS} -DQT_PLUGIN")
add_library(${PLUGIN_NAME} SHARED ${SRCS} resources/sound.qrc)
set_target_properties(${PLUGIN_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ../quick-trays)
target_include_directories(${PLUGIN_NAME} PUBLIC ${DtkWidget_INCLUDE_DIRS}
    ./dbusinterface
    ./dbusinterface/generation_dbus_interface
    ../../interfaces
    ../../widgets
    ../../frame
    ../../frame/util
    ../../frame/accessible
    ../../frame/qtdbusextended
    )

target_link_libraries(${PLUGIN_NAME} PRIVATE
    ${Dtk6Widget_LIBRARIES}
    PkgConfig::XCB_LIBS
    Qt6::Concurrent
    Qt6::Widgets
    Dtk6::Gui
    Dtk6::Widget
    Qt6::DBus
    Qt6::Svg)

install(TARGETS ${PLUGIN_NAME} LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}/plugins/quick-trays)
