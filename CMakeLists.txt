cmake_minimum_required(VERSION 3.5.0)

if(NOT DEFINED CODEGEN_MASTER_PROJECT)
    set(CODEGEN_MASTER_PROJECT OFF)
    if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
        set(CODEGEN_MASTER_PROJECT ON)
    endif()
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(smide)

###############################################################################
# common properties
add_library(runsmide_project_options INTERFACE)
add_library(smide::project_options ALIAS runsmide_project_options)
target_compile_features(runsmide_project_options INTERFACE cxx_std_17)

add_library(runsmide_project_warnings INTERFACE)
add_library(smide::project_warnings ALIAS runsmide_project_warnings)
# todo(Gustav): add extra compiler warnings



###############################################################################
# main lib
function(add_smide_tool)
    set(options)
    set(oneValueArgs NAME)
    set(multiValueArgs FILES)
    cmake_parse_arguments(PARSE_ARGV 0 smide
        "${options}" "${oneValueArgs}" "${multiValueArgs}"
    )

    set(src )
    source_group("" FILES ${smide_FILES})
    add_executable(${smide_NAME} ${smide_FILES})
    target_link_libraries(${smide_NAME}
        PRIVATE
            smide::project_options
            smide::project_warnings
    )
    target_include_directories(${smide_NAME}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
endfunction()

add_smide_tool(
    NAME smide_table
    FILES
        src/smide/table.cc
        src/smide/tinyxml2.cpp
        src/smide/tinyxml2.h
)
add_smide_tool(
    NAME smide_template
    FILES
        src/smide/template.cc
        src/smide/tinyxml2.cpp
        src/smide/tinyxml2.h
)

add_executable(smide::table ALIAS smide_table)

