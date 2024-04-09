set(CMAKE_CXX_STANDARD 11)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

file(GLOB_RECURSE SOURCEFILES "*.h" "*.cpp")
add_library(${PROJECT_NAME} SHARED ${SOURCEFILES})

find_package(Qt6 COMPONENTS Core DBus Network REQUIRED)
find_package(Dtk6 COMPONENTS Core Widget REQUIRED)

target_include_directories(${PROJECT_NAME} PUBLIC
    realize
    ${NETINTERFACEINCLUDE}
    "../common-plugin/"
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::DBus
    Qt6::Network
    Dtk6::Core
    Dtk6::Gui
    Dtk6::Widget
    KF6::NetworkManagerQt
)

set_target_properties(${PROJECT_NAME} PROPERTIES
  VERSION ${CMAKE_PROJECT_VERSION}
  SOVERSION ${CMAKE_PROJECT_VERSION_MAJOR}
)

########### static lib for tests  ###############
add_library(${PROJECT_NAME}_static STATIC
    ${SOURCEFILES}
)
target_link_libraries(${PROJECT_NAME}_static PUBLIC
    Qt6::Core
    Qt6::DBus
    Qt6::Network
    Dtk6::Core
    Dtk6::Gui
    Dtk6::Widget
    KF6::NetworkManagerQt
)
target_include_directories(${PROJECT_NAME}_static PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR};${CMAKE_CURRENT_SOURCE_DIR}/realize>"
)
set_target_properties(${PROJECT_NAME}_static PROPERTIES POSITION_INDEPENDENT_CODE ON)

configure_file(dde-network-core.pc.in ../dde-network-core.pc @ONLY)

set(INTERFACEFILES
    dslcontroller.h
    hotspotcontroller.h
    networkconst.h
    networkcontroller.h
    networkdetails.h
    networkdevicebase.h
    proxycontroller.h
    vpncontroller.h
    wireddevice.h
    wirelessdevice.h)

# install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}Targets LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
# install(FILES ${CMAKE_BINARY_DIR}/dde-network-core.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
# install(FILES ${INTERFACEFILES} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/libddenetworkcore)
# install(FILES ../config/org.deepin.dde.network.json DESTINATION ${CMAKE_INSTALL_DATADIR}/dsg/configs/org.deepin.dde.network)
