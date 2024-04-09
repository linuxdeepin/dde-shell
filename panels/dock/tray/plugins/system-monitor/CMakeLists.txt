set(PLUGIN_NAME "system-monitor")

project("${PLUGIN_NAME}" LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt6 COMPONENTS Core Widgets Gui DBus Concurrent Svg REQUIRED)
find_package(Dtk6Widget REQUIRED)

set(APP_RESOURCES ${CMAKE_CURRENT_LIST_DIR}/asserts/${PLUGIN_NAME}.qrc)

# Sources files
file(GLOB SRCS "*.h" "*.cpp" "gui/*.h" "gui/*.cpp" "dbus/*.h" "dbus/*.cpp")

add_definitions("${QT_DEFINITIONS} -DQT_PLUGIN")
add_library(${PLUGIN_NAME} SHARED ${SRCS} ${APP_RESOURCES})
set_target_properties(${PLUGIN_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ../quick-trays)

target_include_directories(${PLUGIN_NAME} PRIVATE
  ../../interfaces
  ../../widgets
  ../../frame
  ../../frame/util
  ../../frame/qtdbusextended
  ./dbusinterface/generation_dbus_interface
)

target_link_libraries(${PLUGIN_NAME} PRIVATE
  Qt6::DBus
  Qt6::Svg
  Qt6::Widgets
  Dtk6::Widget
)

target_compile_definitions(${PLUGIN_NAME} PRIVATE
    USE_API_QUICKPANEL20
    DDE_DOCK_NEW_VERSION
    USE_MONITOR_POPUP_PLUGIN
)

INSTALL(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION lib/dde-dock/plugins/tmp)
