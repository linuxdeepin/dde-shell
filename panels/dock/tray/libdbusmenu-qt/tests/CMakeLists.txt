if (NOT USE_QT5 AND NOT USE_QT6)
    qt4_automoc(slowmenu.cpp)
endif()
add_executable(slowmenu slowmenu.cpp)

if (NOT USE_QT5 AND NOT USE_QT6)
    target_link_libraries(slowmenu
        ${QT_QTGUI_LIBRARIES}
        ${QT_QTDBUS_LIBRARIES}
        ${QT_QTCORE_LIBRARIES}
        dbusmenu-qt
    )

    set(test_LIBRARIES
        ${QT_QTGUI_LIBRARY}
        ${QT_QTCORE_LIBRARY}
        ${QT_QTDBUS_LIBRARY}
        ${QT_QTTEST_LIBRARY}
        dbusmenu-qt
    )

    include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
        ${CMAKE_CURRENT_BINARY_DIR}/../src
        ${CMAKE_CURRENT_BINARY_DIR}
        ${QT_QTTEST_INCLUDE_DIR}
        ${QT_QTDBUS_INCLUDE_DIR}
        )
elseif(USE_QT5)
        find_package(Qt5Test REQUIRED)
    
        target_link_libraries(slowmenu
            ${Qt5Gui_LIBRARIES}
            ${Qt5Core_LIBRARIES}
            ${Qt5DBus_LIBRARIES}
            dbusmenu-qt5
        )
    
        set(test_LIBRARIES
            ${Qt5Gui_LIBRARIES}
            ${Qt5Core_LIBRARIES}
            ${Qt5DBus_LIBRARIES}
            ${Qt5Test_LIBRARIES}
            dbusmenu-qt5
        )
    
        include_directories(
            ${CMAKE_CURRENT_SOURCE_DIR}/../src
            ${CMAKE_CURRENT_BINARY_DIR}/../src
            ${CMAKE_CURRENT_BINARY_DIR}
            ${Qt5Test_INCLUDE_DIRS}
            ${Qt5DBus_INCLUDE_DIRS}
            )
else()
    find_package(Qt6Test REQUIRED)

    target_link_libraries(slowmenu
        ${Qt6Gui_LIBRARIES}
        ${Qt6Core_LIBRARIES}
        ${Qt6DBus_LIBRARIES}
        dbusmenu-qt6
    )

    set(test_LIBRARIES
        ${Qt6Gui_LIBRARIES}
        ${Qt6Core_LIBRARIES}
        ${Qt6DBus_LIBRARIES}
        ${Qt6Test_LIBRARIES}
        dbusmenu-qt6
    )

    include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
        ${CMAKE_CURRENT_BINARY_DIR}/../src
        ${CMAKE_CURRENT_BINARY_DIR}
        ${Qt6Test_INCLUDE_DIRS}
        ${Qt6DBus_INCLUDE_DIRS}
        )
endif()

# Macros to create "check" target
set(_test_executable_list "")

macro(add_test_executable _executable)
    add_test(${_executable} ${_executable})
    set(_test_executable_list "${_test_executable_list};${_executable}")
    add_executable(${_executable} ${ARGN})
endmacro(add_test_executable)

# Call this at the end
macro(create_check_target)
    add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} --verbose
        DEPENDS ${_test_executable_list})
endmacro(create_check_target)

enable_testing()


# dbusmenuexportertest
set(dbusmenuexportertest_SRCS
    dbusmenuexportertest.cpp
    testutils.cpp
    )

if (NOT USE_QT5 AND NOT USE_QT6)
    qt4_automoc(${dbusmenuexportertest_SRCS})
endif()

add_test_executable(dbusmenuexportertest ${dbusmenuexportertest_SRCS})

target_link_libraries(dbusmenuexportertest
    ${test_LIBRARIES}
    )


# dbusmenuimportertest
set(dbusmenuimportertest_SRCS
    dbusmenuimportertest.cpp
    testutils.cpp
    )

if (NOT USE_QT5 AND NOT USE_QT6)
    qt4_automoc(${dbusmenuimportertest_SRCS})
endif()

add_test_executable(dbusmenuimportertest ${dbusmenuimportertest_SRCS})

target_link_libraries(dbusmenuimportertest
    ${test_LIBRARIES}
    )


# dbusmenushortcuttest
set(dbusmenushortcuttest_SRCS
    dbusmenushortcuttest.cpp
    )

if (NOT USE_QT5 AND NOT USE_QT6)
    qt4_automoc(${dbusmenushortcuttest_SRCS})
endif()

add_test_executable(dbusmenushortcuttest ${dbusmenushortcuttest_SRCS})

target_link_libraries(dbusmenushortcuttest
    ${test_LIBRARIES}
    )

# Keep this at the end
create_check_target()
