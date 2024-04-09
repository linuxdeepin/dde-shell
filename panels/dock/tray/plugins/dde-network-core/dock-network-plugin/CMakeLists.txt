cmake_minimum_required(VERSION 3.7)

set(PLUGIN_NAME "dock-network-plugin")

project(${PLUGIN_NAME})
# 启用 qt moc 的支持
set(CMAKE_AUTOMOC ON)
# 启用 qrc 资源文件的支持
set(CMAKE_AUTORCC ON)

file(GLOB_RECURSE SRCS "*.h" "*.cpp" "item/*.h" "item/*.cpp" "../common-plugin/*.h" "../common-plugin/*.cpp")

find_package(Qt6 COMPONENTS Core Widgets DBus Network LinguistTools REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(Dtk6 REQUIRED COMPONENTS Core)
find_package(Dtk6 REQUIRED COMPONENTS Widget)

file(GLOB TS_FILES "translations/*.ts")
qt_add_translation(QM_FILES ${TS_FILES})
add_custom_target(language ALL DEPENDS ${QM_FILES})

add_definitions("${QT_DEFINITIONS} -DQT_PLUGIN")
add_library(${PLUGIN_NAME} SHARED ${SRCS} ../common-plugin/network.qrc)

target_include_directories(${PLUGIN_NAME} PUBLIC
    ${NETINTERFACEINCLUDE}
    "../common-plugin"
    "../../../interfaces"
)

target_link_libraries(${PLUGIN_NAME} PRIVATE
    dde-network-core_static
)

install(TARGETS ${PLUGIN_NAME} LIBRARY DESTINATION lib/dde-dock/tmp/plugins/quick-trays)
# 安装 .qm 文件
install(FILES ${QM_FILES} DESTINATION ${CMAKE_INSTALL_DATADIR}/${PLUGIN_NAME}/tmp/translations)
