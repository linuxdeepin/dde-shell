cmake_minimum_required(VERSION 3.7)

set(VERSION "2.0.18" CACHE STRING "define project version")
project(dde-network-core
  VERSION ${VERSION}
  DESCRIPTION "DDE Network Core"
  HOMEPAGE_URL "https://github.com/linuxdeepin/dde-network-core"
  LANGUAGES CXX C
)

# 增加安全编译参数
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fstack-protector-all")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-protector-all")
set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -z relro -z now -z noexecstack -pie")

option(ENABLE_DEEPIN_NMQT "enable nmqt patch on deepin" OFF)

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Core)
set(QT_MAJOR_VERSION 6)
find_package(Qt6 COMPONENTS Core Widgets DBus Network LinguistTools REQUIRED)
find_package(KF6NetworkManagerQt REQUIRED)

if (ENABLE_DEEPIN_NMQT)
    add_definitions(-DUSE_DEEPIN_NMQT)
endif()

message(STATUS DEEPIN_NMQT_TEST ": ${DEEPIN_NMQT_TEST}")

add_subdirectory("src")
add_subdirectory("dock-network-plugin")
