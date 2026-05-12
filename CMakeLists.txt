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

    add_library(orc-core INTERFACE)
    target_include_directories(orc-core INTERFACE
        ${ORC_INTREE_SDK_DIR}/orc/common/include
        ${ORC_INTREE_SDK_DIR}/orc/core/include
        ${ORC_INTREE_SDK_DIR}/orc/core/stages
        ${ORC_INTREE_SDK_DIR}/orc/view-types
    )

    # Try to find fmt via CMake config (works with vcpkg toolchain on Windows).
    # Fall back to a Nix store glob for Linux/macOS Nix dev shells.
    find_package(fmt CONFIG QUIET)
    if(fmt_FOUND)
        target_link_libraries(orc-core INTERFACE fmt::fmt-header-only)
    else()
        file(GLOB _orc_fmt_include_candidates "/nix/store/*-fmt-*/include")
        list(LENGTH _orc_fmt_include_candidates _orc_fmt_include_count)
        if(_orc_fmt_include_count GREATER 0)
            list(GET _orc_fmt_include_candidates 0 _orc_fmt_include_dir)
            target_include_directories(orc-core INTERFACE ${_orc_fmt_include_dir})
        endif()
    endif()

    add_library(orc-plugin-sdk INTERFACE)
    target_include_directories(orc-plugin-sdk INTERFACE
        ${ORC_INTREE_SDK_DIR}/orc/sdk/include
    )
    target_link_libraries(orc-plugin-sdk INTERFACE orc-core)

    # When the compiled orc-core library is present (i.e. the host project has
    # been built), link it so in-tree plugins can call into orc-core APIs such
    # as PreviewHelpers that are not header-only.
    if(NOT DEFINED ORC_INTREE_BUILD_DIR)
        set(ORC_INTREE_BUILD_DIR "${ORC_INTREE_SDK_DIR}/build")
    endif()
    find_library(_orc_core_compiled_lib NAMES orc-core
        HINTS "${ORC_INTREE_BUILD_DIR}/lib" "${ORC_INTREE_BUILD_DIR}"
        NO_DEFAULT_PATH
    )
    if(_orc_core_compiled_lib)
        target_link_libraries(orc-core INTERFACE "${_orc_core_compiled_lib}")
    endif()
    unset(_orc_core_compiled_lib CACHE)
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
