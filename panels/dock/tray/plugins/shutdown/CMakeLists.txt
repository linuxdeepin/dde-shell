
set(PLUGIN_NAME "shutdown")

project(${PLUGIN_NAME})

# Sources files
file(GLOB_RECURSE SRCS "*.h" "*.cpp"
    "../../widgets/tipswidget.h"
    "../../widgets/tipswidget.cpp"
    "../../frame/util/imageutil.cpp"
)

find_package(PkgConfig REQUIRED)
find_package(Qt6Widgets REQUIRED)
find_package(Qt6Svg REQUIRED)
find_package(Qt6DBus REQUIRED)
find_package(Dtk6Widget REQUIRED)
find_package(Dtk6Gui REQUIRED)

#if (${CMAKE_SYSTEM_PROCESSOR}  STREQUAL "aarch64")
#    add_definitions("-DDISABLE_POWER_OPTIONS")
#endif()
pkg_check_modules(XCB_EWMH REQUIRED IMPORTED_TARGET x11 xcb xcb-icccm xcb-image xcb-ewmh xcb-composite xtst xext xcursor xkbcommon xres)

add_definitions("${QT_DEFINITIONS} -DQT_PLUGIN")
add_library(${PLUGIN_NAME} SHARED ${SRCS} shutdown.qrc)
set_target_properties(${PLUGIN_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ../)
target_include_directories(${PLUGIN_NAME} PUBLIC ${DtkWidget_INCLUDE_DIRS}
                                                 ../../frame/util
                                                 ../../interfaces
                                                 ../../widgets)
target_link_libraries(${PLUGIN_NAME} PRIVATE
    ${Dtk6Widget_LIBRARIES}
    Qt6::Widgets
    Qt6::DBus
    Qt6::Svg
    Dtk6::Gui
    Dtk6::Widget
    PkgConfig::XCB_EWMH
)

install(TARGETS ${PLUGIN_NAME} LIBRARY DESTINATION lib/dde-dock/tmp/plugins)
