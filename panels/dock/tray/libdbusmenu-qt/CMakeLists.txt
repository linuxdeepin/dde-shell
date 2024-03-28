project(dbusmenu-qt)
cmake_minimum_required(VERSION 2.8.11)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules")

set(CMAKE_CXX_STANDARD 17)

# Build options
option(WITH_DOC "Build documentation (requires Doxygen)" OFF)

# Versions
## Package version
set(dbusmenu_qt_VERSION_MAJOR 0)
set(dbusmenu_qt_VERSION_MINOR 9)
set(dbusmenu_qt_VERSION_PATCH 2)
set(dbusmenu_qt_VERSION ${dbusmenu_qt_VERSION_MAJOR}.${dbusmenu_qt_VERSION_MINOR}.${dbusmenu_qt_VERSION_PATCH})

## Lib version
### Bump this one when a binary-incompatible change is introduced
set(dbusmenu_qt_lib_SOVERSION 2)

### Bump this one when the API is extended in a binary-compatible way
set(dbusmenu_qt_lib_API_VERSION 6)

### Bump this one when changes do not extend the API
set(dbusmenu_qt_lib_PATCH_VERSION 0)

set(dbusmenu_qt_lib_VERSION ${dbusmenu_qt_lib_SOVERSION}.${dbusmenu_qt_lib_API_VERSION}.${dbusmenu_qt_lib_PATCH_VERSION})

# Check if we want to explicitly select the Qt version to be used or autodetect
if (NOT USE_QT4 AND NOT USE_QT5 AND NOT USE_QT6)
    # Autodetect, prefering Qt5
    message(STATUS "Autodetecting Qt version to use")
    find_package(Qt6Widgets QUIET)
    if (Qt6Widgets_FOUND)
        set(USE_QT6 TRUE)
    else()
        find_package(Qt5Widgets QUIET)
        if (Qt5Widgets_FOUND)
            set(USE_QT5 TRUE)
        endif()
    endif()
endif()

# Detect for which Qt version we're building
if (USE_QT6)
    find_package(Qt6Widgets REQUIRED)
    find_package(Qt6DBus REQUIRED)
    include_directories(${Qt6Widgets_INCLUDE_DIRS} ${Qt6DBus_INCLUDE_DIRS})
    find_package(Qt6Core REQUIRED)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOMOC_RELAXED_MODE ON)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)

    set(QT_SUFFIX "qt6")
elseif(USE_QT5)
    find_package(Qt5Widgets REQUIRED)
    find_package(Qt5DBus REQUIRED)
    include_directories(${Qt5Widgets_INCLUDE_DIRS} ${Qt5DBus_INCLUDE_DIRS})
    find_package(Qt5Core REQUIRED)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOMOC_RELAXED_MODE ON)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)

    set(QT_SUFFIX "qt5")
else()
    find_package(Qt4 REQUIRED)
    include_directories(
      ${QT_INCLUDE_DIR}
      ${QT_QTCORE_INCLUDE_DIR}
      ${QT_QTDBUS_INCLUDE_DIR}
      ${QT_QTGUI_INCLUDE_DIR}
      )

    set(QT_SUFFIX "qt")
endif()

include (CheckCXXCompilerFlag)
# Check some compiler flags
check_cxx_compiler_flag(-fvisibility=hidden __DBUSMENU_HAVE_GCC_VISIBILITY)
if (__DBUSMENU_HAVE_GCC_VISIBILITY AND NOT WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
endif (__DBUSMENU_HAVE_GCC_VISIBILITY AND NOT WIN32)

check_cxx_compiler_flag(-Woverloaded-virtual __DBUSMENU_HAVE_W_OVERLOADED_VIRTUAL)
if (__DBUSMENU_HAVE_W_OVERLOADED_VIRTUAL)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Woverloaded-virtual")
endif (__DBUSMENU_HAVE_W_OVERLOADED_VIRTUAL)

check_cxx_compiler_flag(-std=c++11 __DBUSMENU_HAVE_CXX11)
if (__DBUSMENU_HAVE_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif (__DBUSMENU_HAVE_CXX11)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)
set(LIB_DESTINATION "${CMAKE_INSTALL_LIBDIR}")
set(CMAKECONFIG_INSTALL_DIR "${LIB_DESTINATION}/cmake/dbusmenu-${QT_SUFFIX}")
set(INCLUDE_INSTALL_DIR "include/dbusmenu-${QT_SUFFIX}")

# dist targets
set(ARCHIVE_NAME libdbusmenu-${QT_SUFFIX}-${dbusmenu_qt_VERSION})
add_custom_target(dist
    COMMAND bzr export --root=${ARCHIVE_NAME} ${CMAKE_BINARY_DIR}/${ARCHIVE_NAME}.tar.bz2
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )

add_custom_target(distcheck
    COMMAND cd ${CMAKE_BINARY_DIR}
    && rm -rf ${ARCHIVE_NAME}
    && tar xf ${ARCHIVE_NAME}.tar.bz2
    && mkdir ${ARCHIVE_NAME}/build
    && cd ${ARCHIVE_NAME}/build
    && cmake -DCMAKE_INSTALL_PREFIX=../install ..
    && make
    && make install
    && make check
    )
add_dependencies(distcheck dist)

# configure_file(dbusmenu-qt.pc.in dbusmenu-${QT_SUFFIX}.pc @ONLY)

# install(FILES ${CMAKE_BINARY_DIR}/dbusmenu-${QT_SUFFIX}.pc
#         DESTINATION ${LIB_DESTINATION}/pkgconfig
#     )

add_subdirectory(src)
# add_subdirectory(tests)
add_subdirectory(tools)

if(WITH_DOC)
    configure_file(Doxyfile.in ${CMAKE_BINARY_DIR}/Doxyfile @ONLY)

    add_custom_target(doc ALL doxygen
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        )

    install(DIRECTORY ${CMAKE_BINARY_DIR}/html/
        DESTINATION share/doc/libdbusmenu-${QT_SUFFIX}-doc
        )
endif(WITH_DOC)

# Generate dbusmenu-qt-config* files
configure_package_config_file(
    dbusmenu-qt-config.cmake.in
    ${CMAKE_BINARY_DIR}/dbusmenu-${QT_SUFFIX}-config.cmake
    INSTALL_DESTINATION ${CMAKECONFIG_INSTALL_DIR}
    PATH_VARS INCLUDE_INSTALL_DIR
    )

write_basic_package_version_file(
    ${CMAKE_BINARY_DIR}/dbusmenu-${QT_SUFFIX}-config-version.cmake
    VERSION ${dbusmenu_qt_VERSION}
    COMPATIBILITY SameMajorVersion
    )

# # Install dbusmenu-qt-config* files
# install(FILES
#     ${CMAKE_BINARY_DIR}/dbusmenu-${QT_SUFFIX}-config.cmake
#     ${CMAKE_BINARY_DIR}/dbusmenu-${QT_SUFFIX}-config-version.cmake
#     DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
#     COMPONENT Devel
#     )
