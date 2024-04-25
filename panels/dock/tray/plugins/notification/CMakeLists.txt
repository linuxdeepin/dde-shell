set(PLUGIN_NAME "notification")
find_package(Qt6 REQUIRED COMPONENTS DBus)
find_package(Dtk6 REQUIRED COMPONENTS Widget Gui)

add_library(${PLUGIN_NAME} SHARED
    notification.h
    notification.cpp
    notificationplugin.h
    notificationplugin.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../widgets/tipswidget.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../widgets/tipswidget.cpp
    notification.qrc
)
target_compile_definitions(${PLUGIN_NAME} PRIVATE QT_PLUGIN)
set_target_properties(${PLUGIN_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ..)

target_include_directories(${PLUGIN_NAME} PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../interfaces>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../widgets>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../frame/qtdbusextended>
)

target_link_libraries(${PLUGIN_NAME} PRIVATE
    Dtk6::Widget
    Qt6::DBus
    # Dtk6::Widget
    Dtk6::Gui
)

install(TARGETS ${PLUGIN_NAME} LIBRARY DESTINATION lib/dde-dock/tmp/plugins)
