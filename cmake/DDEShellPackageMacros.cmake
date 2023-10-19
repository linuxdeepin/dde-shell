include(CMakeParseArguments)

macro(ds_build_package)
    set(oneValueArgs PACKAGE TARGET PACKAGE_ROOT_DIR)
    cmake_parse_arguments(_config "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(package_root_dir ${CMAKE_CURRENT_SOURCE_DIR}/package)
    if (DEFINED _config_PACKAGE_ROOT_DIR)
        set(package_root_dir ${_config_PACKAGE_ROOT_DIR})
    endif()
    file(GLOB_RECURSE package_files ${package_root_dir}/*)
    add_custom_target(${_config_PACKAGE}_package ALL
        SOURCES ${package_files}
    )
    set(package_dirs ${PROJECT_BINARY_DIR}/packages/${_config_PACKAGE}/)
    add_custom_command(TARGET ${_config_PACKAGE}_package
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${package_root_dir} ${package_dirs}
    )

    if (DEFINED _config_TARGET)
        set_target_properties(${_config_TARGET} PROPERTIES
            PREFIX ""
            OUTPUT_NAME ${_config_PACKAGE}
            LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/plugins/
        )
    endif()
endmacro()

function(ds_install_package)
    ds_build_package(${ARGN})
    install(DIRECTORY ${package_dirs} DESTINATION ${DDE_SHELL_PACKAGE_INSTALL_DIR}/${_config_PACKAGE})

    if (DEFINED _config_TARGET)
        install(TARGETS ${_config_TARGET} DESTINATION ${DDE_SHELL_PLUGIN_INSTALL_DIR}/)
    endif()
endfunction()
