cmake_minimum_required(VERSION 3.20)

project(orc-plugin-skeleton
    VERSION 0.1.0
    DESCRIPTION "Decode-Orc external plugin skeleton"
    LANGUAGES CXX
)

set(ORC_PLUGIN_VERSION "${PROJECT_VERSION}" CACHE STRING
    "Plugin version string embedded in StagePluginDescriptor")

# On tagged CI builds, derive plugin version from tag names like v1.2.3
# unless the caller explicitly overrides ORC_PLUGIN_VERSION.
if(ORC_PLUGIN_VERSION STREQUAL PROJECT_VERSION)
    if(DEFINED ENV{GITHUB_REF_TYPE} AND "$ENV{GITHUB_REF_TYPE}" STREQUAL "tag" AND DEFINED ENV{GITHUB_REF_NAME})
        set(_orc_ref_name "$ENV{GITHUB_REF_NAME}")
        if(_orc_ref_name MATCHES "^v(.+)$")
            set(ORC_PLUGIN_VERSION "${CMAKE_MATCH_1}")
            set(ORC_PLUGIN_VERSION "${ORC_PLUGIN_VERSION}" CACHE STRING
                "Plugin version string embedded in StagePluginDescriptor" FORCE)
        endif()
    endif()
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_TESTS "Build skeleton plugin tests" ON)

if(DEFINED ORC_INTREE_SDK_DIR)
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/DecodeOrcPluginSDKHelpers.cmake")

    # Since Decode-Orc 2.0 the SDK include surface is fully self-contained
    # under orc/sdk/include (<orc/plugin/...> and <orc/stage/...>); private
    # host include trees must not be exposed to plugin code.
    add_library(orc-sdk-includes INTERFACE)

    # Try to find fmt via CMake config (works with vcpkg toolchain on Windows).
    # Fall back to a Nix store glob for Linux/macOS Nix dev shells.
    find_package(fmt CONFIG QUIET)
    if(fmt_FOUND)
        target_link_libraries(orc-sdk-includes INTERFACE fmt::fmt-header-only)
    else()
        file(GLOB _orc_fmt_include_candidates "/nix/store/*-fmt-*/include")
        list(LENGTH _orc_fmt_include_candidates _orc_fmt_include_count)
        if(_orc_fmt_include_count GREATER 0)
            list(GET _orc_fmt_include_candidates 0 _orc_fmt_include_dir)
            target_include_directories(orc-sdk-includes INTERFACE ${_orc_fmt_include_dir})
        endif()
    endif()

    add_library(orc-plugin-sdk INTERFACE)
    target_include_directories(orc-plugin-sdk INTERFACE
        ${ORC_INTREE_SDK_DIR}/orc/sdk/include
    )
    target_link_libraries(orc-plugin-sdk INTERFACE orc-sdk-includes)
else()
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/DecodeOrcPluginSDKHelpers.cmake")
    find_package(decode-orc-plugin-sdk REQUIRED)
endif()

set(SKELETON_STAGE_NAME "skeleton_passthrough")
set(SKELETON_PLUGIN_TARGET "orc-stage-plugin-skeleton-passthrough")

orc_add_stage_plugin(
    ${SKELETON_PLUGIN_TARGET}
    OUTPUT_NAME ${SKELETON_PLUGIN_TARGET}
    PLUGIN_VERSION ${ORC_PLUGIN_VERSION}
    SOURCES
        src/plugin.cpp
        src/skeleton_passthrough_stage.cpp
)

target_include_directories(${SKELETON_PLUGIN_TARGET} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
