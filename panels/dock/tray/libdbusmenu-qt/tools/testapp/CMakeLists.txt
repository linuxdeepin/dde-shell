set(qtapp_SRCS
    main.cpp
    )

add_executable(dbusmenubench-qtapp ${qtapp_SRCS})

if (NOT USE_QT5 AND NOT USE_QT6))
    # Qt4
    include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/../../src
        ${CMAKE_CURRENT_BINARY_DIR}/../../src
        ${QT_INCLUDE_DIR}
        ${QT_QTCORE_INCLUDE_DIR}
        ${QT_QTGUI_INCLUDE_DIR}
        ${QT_QTDBUS_INCLUDE_DIR}
        ${QJSON_INCLUDE_DIR}
        )

    target_link_libraries(dbusmenubench-qtapp
        dbusmenu-qt
        ${QT_QTGUI_LIBRARY}
        ${QT_QTCORE_LIBRARY}
        ${QT_QTDBUS_LIBRARY}
        ${QJSON_LIBRARIES}
        )
elseif(USE_QT5)
   # Qt5
    include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/../../src
        ${CMAKE_CURRENT_BINARY_DIR}/../../src
        ${Qt5Widgets_INCLUDE_DIRS}
        ${Qt5Core_INCLUDE_DIRS}
        ${Qt5Gui_INCLUDE_DIRS}
        ${Qt5DBus_INCLUDE_DIRS}
        ${QJSON_INCLUDE_DIR}
        )
   
    target_link_libraries(dbusmenubench-qtapp
        dbusmenu-qt5
        ${Qt5Gui_LIBRARIES}
        ${Qt5Core_LIBRARIES}
        ${Qt5DBus_LIBRARIES}
        ${Qt5Widgets_LIBRARIES}
        ${QJSON_LIBRARIES}
        )
else()
   # Qt6
   include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
    ${CMAKE_CURRENT_BINARY_DIR}/../../src
    ${Qt6Widgets_INCLUDE_DIRS}
    ${Qt6Core_INCLUDE_DIRS}
    ${Qt6Gui_INCLUDE_DIRS}
    ${Qt6DBus_INCLUDE_DIRS}
    ${QJSON_INCLUDE_DIR}
    )

target_link_libraries(dbusmenubench-qtapp
    dbusmenu-qt6
    ${Qt6Gui_LIBRARIES}
    ${Qt6Core_LIBRARIES}
    ${Qt6DBus_LIBRARIES}
    ${Qt6Widgets_LIBRARIES}
    ${QJSON_LIBRARIES}
    )
endif()
