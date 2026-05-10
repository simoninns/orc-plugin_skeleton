if(DEFINED ORC_INTREE_SDK_DIR AND TARGET orc-plugin-sdk)
    set(SKELETON_TEST_SDK_TARGET orc-plugin-sdk)
elseif(TARGET orc::plugin-sdk)
    set(SKELETON_TEST_SDK_TARGET orc::plugin-sdk)
elseif(TARGET orc-plugin-sdk)
    set(SKELETON_TEST_SDK_TARGET orc-plugin-sdk)
else()
    message(FATAL_ERROR "Neither orc::plugin-sdk nor orc-plugin-sdk is available for tests")
endif()

add_executable(skeleton_stage_smoke_test
    stage_smoke_test.cpp
    ../src/skeleton_passthrough_stage.cpp
)

target_link_libraries(skeleton_stage_smoke_test PRIVATE
    ${SKELETON_TEST_SDK_TARGET}
)

target_include_directories(skeleton_stage_smoke_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

add_test(NAME skeleton_stage_smoke_test COMMAND skeleton_stage_smoke_test)

add_executable(skeleton_plugin_entrypoints_test
    plugin_entrypoints_test.cpp
    ../src/plugin.cpp
    ../src/skeleton_passthrough_stage.cpp
)

target_link_libraries(skeleton_plugin_entrypoints_test PRIVATE
    ${SKELETON_TEST_SDK_TARGET}
)

target_include_directories(skeleton_plugin_entrypoints_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

add_test(NAME skeleton_plugin_entrypoints_test COMMAND skeleton_plugin_entrypoints_test)
