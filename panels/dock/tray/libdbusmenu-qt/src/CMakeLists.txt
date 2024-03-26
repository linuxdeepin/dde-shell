include(CheckCXXSourceCompiles)

check_cxx_compiler_flag(-Wall __DBUSMENU_HAVE_W_ALL)
if (__DBUSMENU_HAVE_W_ALL)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
endif (__DBUSMENU_HAVE_W_ALL)

# Check some compiler flags
check_cxx_compiler_flag(-fvisibility=hidden __DBUSMENU_HAVE_GCC_VISIBILITY)
if (__DBUSMENU_HAVE_GCC_VISIBILITY AND NOT WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
endif (__DBUSMENU_HAVE_GCC_VISIBILITY AND NOT WIN32)

check_cxx_compiler_flag(-Woverloaded-virtual __DBUSMENU_HAVE_W_OVERLOADED_VIRTUAL)
if (__DBUSMENU_HAVE_W_OVERLOADED_VIRTUAL)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Woverloaded-virtual")
endif (__DBUSMENU_HAVE_W_OVERLOADED_VIRTUAL)

check_cxx_compiler_flag(-Wall __DBUSMENU_HAVE_W_ALL)
if (__DBUSMENU_HAVE_W_ALL)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
endif (__DBUSMENU_HAVE_W_ALL)

check_cxx_compiler_flag(-std=c++11 __DBUSMENU_HAVE_CXX11)
if (__DBUSMENU_HAVE_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif (__DBUSMENU_HAVE_CXX11)

# Check whether QIcon::name() exists. It was added in late Qt 4.7 cycle, and is
# not present in betas.

if (NOT USE_QT5 AND NOT USE_QT6)
    set(CMAKE_REQUIRED_INCLUDES "${QT_INCLUDE_DIR}")
    set(CMAKE_REQUIRED_LIBRARIES "${QT_QTGUI_LIBRARIES};${QT_QTCORE_LIBRARIES}")
elseif(USE_QT5)
    set(CMAKE_REQUIRED_INCLUDES "${Qt5Gui_INCLUDE_DIRS};${Qt5Core_INCLUDE_DIRS}")
    set(CMAKE_REQUIRED_LIBRARIES "${Qt5Gui_LIBRARIES};${Qt5Core_LIBRARIES}")
else()
    set(CMAKE_REQUIRED_INCLUDES "${Qt6Gui_INCLUDE_DIRS};${Qt6Core_INCLUDE_DIRS}")
    set(CMAKE_REQUIRED_LIBRARIES "${Qt6Gui_LIBRARIES};${Qt6Core_LIBRARIES}")
endif()
check_cxx_source_compiles("
#include <QtGui/QIcon>
int main() {
    QIcon icon;
    icon.name();
    return 0;
}
" HAVE_QICON_NAME)
if (NOT HAVE_QICON_NAME)
    message(STATUS "QIcon::name() does not exist, DBusMenuExporter will not export icon names by itself")
endif()
configure_file(dbusmenu_config.h.in dbusmenu_config.h @ONLY)

set(dbusmenu_qt_SRCS
    dbusmenu_p.cpp
    dbusmenuexporter.cpp
    dbusmenuexporterdbus_p.cpp
    dbusmenuimporter.cpp
    dbusmenutypes_p.cpp
    dbusmenushortcut_p.cpp
    utils.cpp
    )

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
    )

if (NOT USE_QT5 AND NOT USE_QT6)
    qt4_automoc(${dbusmenu_qt_SRCS})
    qt4_add_dbus_adaptor(dbusmenu_qt_SRCS
        ${CMAKE_CURRENT_SOURCE_DIR}/com.canonical.dbusmenu.xml
        ${CMAKE_CURRENT_SOURCE_DIR}/dbusmenuexporterdbus_p.h DBusMenuExporterDBus
        )
elseif(USE_QT5)
    qt5_add_dbus_adaptor(dbusmenu_qt_SRCS
        ${CMAKE_CURRENT_SOURCE_DIR}/com.canonical.dbusmenu.xml
        ${CMAKE_CURRENT_SOURCE_DIR}/dbusmenuexporterdbus_p.h DBusMenuExporterDBus
        )
else()
    qt6_add_dbus_adaptor(dbusmenu_qt_SRCS
        ${CMAKE_CURRENT_SOURCE_DIR}/com.canonical.dbusmenu.xml
        ${CMAKE_CURRENT_SOURCE_DIR}/dbusmenuexporterdbus_p.h DBusMenuExporterDBus
        )
endif()

configure_file(dbusmenu_version.h.in dbusmenu_version.h  @ONLY)

add_library(dbusmenu-${QT_SUFFIX} SHARED ${dbusmenu_qt_SRCS})
set_target_properties(dbusmenu-${QT_SUFFIX} PROPERTIES
    VERSION ${dbusmenu_qt_lib_VERSION}
    SOVERSION ${dbusmenu_qt_lib_SOVERSION}
    )


if (NOT USE_QT5 AND NOT USE_QT6)
    target_link_libraries(dbusmenu-${QT_SUFFIX}
        ${QT_QTGUI_LIBRARIES}
        ${QT_QTDBUS_LIBRARIES}
        ${QT_QTCORE_LIBRARIES}
        )
elseif(USE_QT5)
    target_link_libraries(dbusmenu-${QT_SUFFIX}
        ${Qt5Gui_LIBRARIES}
        ${Qt5Core_LIBRARIES}
        ${Qt5DBus_LIBRARIES}
        ${Qt5Widgets_LIBRARIES}
        )
else()
    target_link_libraries(dbusmenu-${QT_SUFFIX}
        ${Qt6Gui_LIBRARIES}
        ${Qt6Core_LIBRARIES}
        ${Qt6DBus_LIBRARIES}
        ${Qt6Widgets_LIBRARIES}
        )
endif()

# Make sure linking to the target adds dbusmenu-qt install directory
target_include_directories(dbusmenu-${QT_SUFFIX}
    INTERFACE "$<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}>")

# install(TARGETS dbusmenu-${QT_SUFFIX}
#     EXPORT dbusmenu-${QT_SUFFIX}-targets
#     LIBRARY DESTINATION ${LIB_DESTINATION}
#     RUNTIME DESTINATION bin
#     )

# install(EXPORT dbusmenu-${QT_SUFFIX}-targets
#     DESTINATION ${CMAKECONFIG_INSTALL_DIR})

# install(DIRECTORY .
#     DESTINATION ${INCLUDE_INSTALL_DIR}
#     FILES_MATCHING PATTERN "*.h"
#     PATTERN "*_p.h" EXCLUDE
#     )

# install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dbusmenu_version.h
#     DESTINATION ${INCLUDE_INSTALL_DIR}
#     )
