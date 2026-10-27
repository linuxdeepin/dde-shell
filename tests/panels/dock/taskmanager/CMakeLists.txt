# SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: CC0-1.0

find_package(GTest REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} ${REQUIRED_QT_VERSION} REQUIRED COMPONENTS Core Test)

include(GoogleTest)

add_executable(rolecombinemodel_tests
    ${CMAKE_SOURCE_DIR}/panels/dock/taskmanager/rolecombinemodel.h
    ${CMAKE_SOURCE_DIR}/panels/dock/taskmanager/rolecombinemodel.cpp
    combinemodela.cpp
    combinemodela.h
    combinemodelb.cpp
    combinemodelb.h
    rolecombinemodeltests.cpp
)

target_link_libraries(rolecombinemodel_tests
    GTest::GTest
    GTest::Main
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Test
)
target_include_directories(rolecombinemodel_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/panels/dock/taskmanager/
)

gtest_discover_tests(rolecombinemodel_tests)

add_executable(rolegroupmodel_tests
    ${CMAKE_SOURCE_DIR}/panels/dock/taskmanager/rolegroupmodel.h
    ${CMAKE_SOURCE_DIR}/panels/dock/taskmanager/rolegroupmodel.cpp
    rolegroupmodeltests.cpp
)

target_link_libraries(rolegroupmodel_tests
    GTest::GTest
    GTest::Main
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Test
    # dock-taskmanager
)
target_include_directories(rolegroupmodel_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/panels/dock/taskmanager/
)

gtest_discover_tests(rolegroupmodel_tests)

# Folder-source menu extensions are tested without a desktop or applet service.
add_executable(foldermenuregistry_tests
    foldermenuregistrytests.cpp
    ${CMAKE_SOURCE_DIR}/panels/dock/taskmanager/foldermenuregistry.cpp
    ${CMAKE_SOURCE_DIR}/panels/dock/taskmanager/foldermenuregistry.h
)
target_include_directories(foldermenuregistry_tests PRIVATE ${CMAKE_SOURCE_DIR}/panels/dock/taskmanager)
target_link_libraries(foldermenuregistry_tests PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Test)
add_test(NAME folder_menu_registry COMMAND foldermenuregistry_tests)

# Exercise the real backend through Applet's QML attached-type lookup, not just
# QObject/QMetaObject invocation (capitalized D-Bus methods differ here).
add_executable(taskmanagerqmlapi_tests taskmanagerqmlapitests.cpp)
# Inject the folder model without starting window monitoring or D-Bus services,
# as in the other dock tests that access private model state.
target_compile_options(taskmanagerqmlapi_tests PRIVATE -fno-access-control)
add_library(test-folder-provider MODULE testfolderprovider.cpp)
target_link_libraries(test-folder-provider PRIVATE dde-shell-dock)
set_target_properties(test-folder-provider PROPERTIES PREFIX ""
    OUTPUT_NAME "org.deepin.ds.folderprovider.testsource"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/plugins")
target_compile_definitions(taskmanagerqmlapi_tests PRIVATE
    TEST_FOLDER_PLUGIN_DIR="${CMAKE_CURRENT_BINARY_DIR}/plugins"
    TEST_SHELL_QML_DIR="${CMAKE_BINARY_DIR}/plugins"
    TEST_TASKMANAGER_PACKAGE="${CMAKE_SOURCE_DIR}/panels/dock/taskmanager/package")
add_dependencies(taskmanagerqmlapi_tests test-folder-provider)
target_include_directories(taskmanagerqmlapi_tests PRIVATE ${CMAKE_SOURCE_DIR}/panels/dock/taskmanager)
target_link_libraries(taskmanagerqmlapi_tests PRIVATE dock-taskmanager dde-shell-dock Qt${QT_VERSION_MAJOR}::Qml Qt${QT_VERSION_MAJOR}::Test)
add_test(NAME taskmanager_qml_api COMMAND taskmanagerqmlapi_tests)
# Debian builds disable build RPATH; use the plugin and frame from this build.
set_tests_properties(taskmanager_qml_api PROPERTIES ENVIRONMENT
    "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software;LD_LIBRARY_PATH=$<TARGET_FILE_DIR:dock-taskmanager>:$<TARGET_FILE_DIR:dde-shell-dock>:$<TARGET_FILE_DIR:dde-shell-frame>")
