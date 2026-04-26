cmake_minimum_required(VERSION 3.25)

### PROJECT ###

project(toolkit)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(TOOLKIT_ENABLE_INSTALL "Enable Install Targets" OFF)

### CORE ###

file(GLOB_RECURSE CORE_SOURCES src/core/*.cxx)
add_library(toolkit_core ${CORE_SOURCES})
add_library(toolkit::core ALIAS toolkit_core)
target_include_directories(toolkit_core PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>
)

### JSON ###

file(GLOB_RECURSE JSON_SOURCES src/json/*.cxx)
add_library(toolkit_json ${JSON_SOURCES})
add_library(toolkit::json ALIAS toolkit_json)
target_include_directories(toolkit_json PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>
)
target_link_libraries(toolkit_json PUBLIC toolkit_core)

### TOML ###

file(GLOB_RECURSE TOML_SOURCES src/toml/*.cxx)
add_library(toolkit_toml ${TOML_SOURCES})
add_library(toolkit::toml ALIAS toolkit_toml)
target_include_directories(toolkit_toml PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>
)
target_link_libraries(toolkit_toml PUBLIC toolkit_core)

### INSTALL ###

if (TOOLKIT_ENABLE_INSTALL)
    install(TARGETS toolkit_core toolkit_json toolkit_toml
            EXPORT toolkit_targets
            ARCHIVE DESTINATION "${CMAKE_INSTALL_PREFIX}/lib"
            LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}/lib"
            RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/bin"
            INCLUDES DESTINATION "${CMAKE_INSTALL_PREFIX}/include"
    )

    install(EXPORT toolkit_targets
            FILE toolkit_targets.cmake
            NAMESPACE toolkit::
            DESTINATION lib/cmake/toolkit
    )
endif ()
