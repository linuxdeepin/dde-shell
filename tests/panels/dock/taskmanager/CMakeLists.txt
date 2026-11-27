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

# The mirror mode approval policy is a pure function of the switch and the listed flag.
add_executable(mirrormodepolicy_tests
    mirrormodepolicytests.cpp
    ${CMAKE_SOURCE_DIR}/panels/dock/taskmanager/mirrormodepolicy.h
)
target_include_directories(mirrormodepolicy_tests PRIVATE ${CMAKE_SOURCE_DIR}/panels/dock/taskmanager)
target_link_libraries(mirrormodepolicy_tests PRIVATE GTest::GTest GTest::Main)
add_test(NAME mirror_mode_policy COMMAND mirrormodepolicy_tests)
