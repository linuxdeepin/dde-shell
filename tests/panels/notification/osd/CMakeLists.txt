# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(GTest REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} ${REQUIRED_QT_VERSION} REQUIRED COMPONENTS
    Core
    Test
    DBus
)

# ---- osdpanel_test ----
add_executable(osdpanel_tests
    ${CMAKE_SOURCE_DIR}/panels/notification/osd/osdpanel.h
    ${CMAKE_SOURCE_DIR}/panels/notification/osd/osdpanel.cpp
    ${CMAKE_SOURCE_DIR}/panels/notification/osd/osddbusadaptor.h
    ${CMAKE_SOURCE_DIR}/panels/notification/osd/osddbusadaptor.cpp
    osdpanel_test.cpp
)

set_property(TARGET osdpanel_tests APPEND PROPERTY AUTOMOC_MACRO_NAMES "D_APPLET_CLASS")

target_compile_options(osdpanel_tests PRIVATE
    -fvisibility=hidden
    -fvisibility-inlines-hidden
)

target_include_directories(osdpanel_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/panels/notification/osd
    ${CMAKE_SOURCE_DIR}/frame
)

target_link_libraries(osdpanel_tests PRIVATE
    GTest::GTest
    GTest::gmock
    GTest::gmock_main
    GTest::Main

    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Test
    Qt${QT_VERSION_MAJOR}::DBus

    dde-shell-frame
)

add_test(
    NAME osdpanel_tests
    COMMAND ${CMAKE_COMMAND} -E env
        LD_LIBRARY_PATH=${CMAKE_BINARY_DIR}/frame
        $<TARGET_FILE:osdpanel_tests>
)

# ---- audioapplet_test ----
add_executable(audioapplet_tests
    ${CMAKE_SOURCE_DIR}/panels/notification/osd/audio/audioapplet.h
    ${CMAKE_SOURCE_DIR}/panels/notification/osd/audio/audioapplet.cpp
    audioapplet_test.cpp
)

set_property(TARGET audioapplet_tests APPEND PROPERTY AUTOMOC_MACRO_NAMES "D_APPLET_CLASS")

target_compile_options(audioapplet_tests PRIVATE
    -fvisibility=hidden
    -fvisibility-inlines-hidden
)

target_include_directories(audioapplet_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/panels/notification/osd/audio
    ${CMAKE_SOURCE_DIR}/frame
)

target_link_libraries(audioapplet_tests PRIVATE
    GTest::GTest
    GTest::gmock
    GTest::gmock_main
    GTest::Main

    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Test
    Qt${QT_VERSION_MAJOR}::DBus

    dde-shell-frame
)

add_test(
    NAME audioapplet_tests
    COMMAND ${CMAKE_COMMAND} -E env
        LD_LIBRARY_PATH=${CMAKE_BINARY_DIR}/frame
        $<TARGET_FILE:audioapplet_tests>
)

# ---- brightnessapplet_test ----
find_package(TreelandProtocols 0.6 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(WaylandClient REQUIRED IMPORTED_TARGET wayland-client)

add_executable(brightnessapplet_tests
    ${CMAKE_SOURCE_DIR}/panels/notification/osd/brightness/brightnessapplet.h
    ${CMAKE_SOURCE_DIR}/panels/notification/osd/brightness/brightnessapplet.cpp
    ${CMAKE_SOURCE_DIR}/panels/notification/osd/brightness/treelandbrightness.h
    ${CMAKE_SOURCE_DIR}/panels/notification/osd/brightness/treelandbrightness.cpp
    brightnessapplet_test.cpp
)

qt_generate_wayland_protocol_client_sources(brightnessapplet_tests
    NO_INCLUDE_CORE_ONLY
    FILES
        ${TREELAND_PROTOCOLS_DATA_DIR}/treeland-output-manager-unstable-v2.xml
)

set_property(TARGET brightnessapplet_tests APPEND PROPERTY AUTOMOC_MACRO_NAMES "D_APPLET_CLASS")

target_compile_options(brightnessapplet_tests PRIVATE
    -fvisibility=hidden
    -fvisibility-inlines-hidden
)

target_include_directories(brightnessapplet_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/panels/notification/osd/brightness
    ${CMAKE_SOURCE_DIR}/frame
)

target_link_libraries(brightnessapplet_tests PRIVATE
    GTest::GTest
    GTest::gmock
    GTest::gmock_main
    GTest::Main

    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Test
    Qt${QT_VERSION_MAJOR}::DBus
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::WaylandClient

    dde-shell-frame
    PkgConfig::WaylandClient
)

add_test(
    NAME brightnessapplet_tests
    COMMAND ${CMAKE_COMMAND} -E env
        LD_LIBRARY_PATH=${CMAKE_BINARY_DIR}/frame
        $<TARGET_FILE:brightnessapplet_tests>
)

# ---- displaymodeapplet_test ----
add_executable(displaymodeapplet_tests
    ${CMAKE_SOURCE_DIR}/panels/notification/osd/displaymode/displaymodeapplet.h
    ${CMAKE_SOURCE_DIR}/panels/notification/osd/displaymode/displaymodeapplet.cpp
    displaymodeapplet_test.cpp
)

set_property(TARGET displaymodeapplet_tests APPEND PROPERTY AUTOMOC_MACRO_NAMES "D_APPLET_CLASS")

target_compile_options(displaymodeapplet_tests PRIVATE
    -fvisibility=hidden
    -fvisibility-inlines-hidden
)

target_include_directories(displaymodeapplet_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/panels/notification/osd/displaymode
    ${CMAKE_SOURCE_DIR}/frame
)

target_link_libraries(displaymodeapplet_tests PRIVATE
    GTest::GTest
    GTest::gmock
    GTest::gmock_main
    GTest::Main

    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Test
    Qt${QT_VERSION_MAJOR}::DBus

    dde-shell-frame
)

add_test(
    NAME displaymodeapplet_tests
    COMMAND ${CMAKE_COMMAND} -E env
        LD_LIBRARY_PATH=${CMAKE_BINARY_DIR}/frame
        $<TARGET_FILE:displaymodeapplet_tests>
)
