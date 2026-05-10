#include "orc_plugin_sdk_compat.h"

#include <iostream>

ORC_STAGE_PLUGIN_EXPORT const orc::StagePluginDescriptor* orc_get_stage_plugin_descriptor();
ORC_STAGE_PLUGIN_EXPORT bool orc_register_stage_plugin(
    const orc::OrcPluginServices* services,
    void* context,
    bool (*register_stage)(void* context, const char* stage_name, orc::OrcStageFactoryFn factory),
    const char** error_message);

namespace {

bool register_probe(void* context, const char* stage_name, orc::OrcStageFactoryFn)
{
    auto* called = static_cast<bool*>(context);
    if (called) {
        *called = true;
    }
    return std::string(stage_name) == "skeleton_passthrough";
}

} // namespace

int main()
{
    const auto* descriptor = orc_get_stage_plugin_descriptor();
    if (!descriptor) {
        std::cerr << "Descriptor entrypoint returned null\n";
        return 1;
    }

    if (std::string(descriptor->plugin_id) != "org.decodeorc.stage.skeleton_passthrough") {
        std::cerr << "Unexpected plugin_id\n";
        return 1;
    }

    if (descriptor->host_abi_version != orc::kStagePluginHostAbiVersion) {
        std::cerr << "Host ABI mismatch\n";
        return 1;
    }

    bool called = false;
    const char* error_message = nullptr;
    if (!orc_register_stage_plugin(nullptr, &called, &register_probe, &error_message)) {
        std::cerr << "Register entrypoint failed: "
                  << (error_message ? error_message : "<none>") << '\n';
        return 1;
    }

    if (!called) {
        std::cerr << "Register callback was not called\n";
        return 1;
    }

    return 0;
}
