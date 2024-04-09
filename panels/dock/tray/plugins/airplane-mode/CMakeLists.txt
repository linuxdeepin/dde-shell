
set(PLUGIN_NAME "airplane-mode")

project(${PLUGIN_NAME})

generation_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/dbusinterface/xml ${CMAKE_CURRENT_SOURCE_DIR}/dbusinterface/generation_dbus_interface)

# Sources files
file(GLOB_RECURSE SRCS "*.h"
    "*.cpp"
    "../../widgets/tipswidget.h"
    "../../widgets/tipswidget.cpp"
    "../../frame/util/imageutil.h"
    "../../frame/util/imageutil.cpp"
    "../../frame/qtdbusextended/*.h"
    "../../frame/qtdbusextended/*.cpp")

find_package(PkgConfig REQUIRED)
find_package(Qt6Widgets REQUIRED)
find_package(Qt6Svg REQUIRED)
find_package(Qt6DBus REQUIRED)
find_package(Dtk6Gui REQUIRED)
find_package(Dtk6Widget REQUIRED)

pkg_check_modules(XCB_LIBS REQUIRED IMPORTED_TARGET xcursor)

add_definitions("${QT_DEFINITIONS} -DQT_PLUGIN")
add_library(${PLUGIN_NAME} SHARED ${SRCS} resources/airplane_mode.qrc)
set_target_properties(${PLUGIN_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ..)

target_include_directories(${PLUGIN_NAME} PUBLIC ${DtkWidget_INCLUDE_DIRS}
  ../../interfaces
  ../../widgets
  ../../frame
  ../../frame/util
  ../../frame/qtdbusextended
  ./dbusinterface/generation_dbus_interface)

target_link_libraries(${PLUGIN_NAME} PRIVATE
  ${Dtk6Widget_LIBRARIES}
  PkgConfig::XCB_LIBS
  Qt6::Widgets
  Qt6::DBus
  Dtk6::Widget
  Dtk6::Gui
  Qt6::Svg)

install(TARGETS ${PLUGIN_NAME} LIBRARY DESTINATION lib/dde-dock/tmp/plugins)
