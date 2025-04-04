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
set(src
    src/smide/table.cc
    src/smide/tinyxml2.cpp
    src/smide/tinyxml2.h
)
source_group("" FILES ${src})
add_executable(smide ${src})
target_link_libraries(smide
    PRIVATE
        smide::project_options
        smide::project_warnings
)
target_include_directories(smide
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
)
add_executable(smide::smide ALIAS smide)

