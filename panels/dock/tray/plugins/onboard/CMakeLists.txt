
set(PLUGIN_NAME "onboard")

project(${PLUGIN_NAME})

# Sources files
file(GLOB_RECURSE SRCS "*.h"
    "*.cpp"
    ${CMAKE_CURRENT_SOURCE_DIR}/../../widgets/tipswidget.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../widgets/tipswidget.cpp
    "../../frame/qtdbusextended/*.h"
    "../../frame/qtdbusextended/*.cpp")

find_package(PkgConfig REQUIRED)
find_package(Qt6Widgets REQUIRED)
find_package(Qt6Gui REQUIRED)
find_package(Qt6Svg REQUIRED)
find_package(Qt6DBus REQUIRED)
find_package(Dtk6Gui REQUIRED)
find_package(Dtk6Widget REQUIRED)

add_definitions("${QT_DEFINITIONS} -DQT_PLUGIN")
add_library(${PLUGIN_NAME} SHARED ${SRCS} onboard.qrc)
set_target_properties(${PLUGIN_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ../)

target_include_directories(${PLUGIN_NAME} PUBLIC ${Qt6DBus_INCLUDE_DIRS}
    ../../interfaces
    ../../frame/qtdbusextended)

target_link_libraries(${PLUGIN_NAME} PRIVATE
    Qt6::Widgets
    Qt6::Gui
    Qt6::DBus
    Qt6::Svg
    Dtk6::Widget
    Dtk6::Gui
)

install(TARGETS ${PLUGIN_NAME} LIBRARY DESTINATION lib/dde-dock/tmp/plugins)
