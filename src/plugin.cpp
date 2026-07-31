/*
 * File:        plugin.cpp
 * Module:      orc-stage-plugin-skeleton-passthrough
 * Purpose:     Runtime plugin bundle for SkeletonPassthroughStage
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 <Your Name>
 */

#include "plugin.h"
#include "skeleton_passthrough_stage.h"

namespace {

orc::DAGStagePtr create_stage()
{
    return std::make_shared<orc::plugins::skeleton::SkeletonPassthroughStage>();
}

} // namespace

ORC_STAGE_PLUGIN_EXPORT const orc::StagePluginDescriptor* orc_get_stage_plugin_descriptor()
{
    return &orc::plugins::skeleton::kPluginDescriptor;
}

ORC_STAGE_PLUGIN_EXPORT bool orc_register_stage_plugin(
    const orc::OrcPluginServices* services,
    void* context,
    bool (*register_stage)(void* context, const char* stage_name, orc::OrcStageFactoryFn factory),
    const char** error_message)
{
    orc::plugin::set_services(services);

    if (!register_stage) {
        if (error_message) {
            *error_message = "Missing stage registration callback";
        }
        return false;
    }

    const auto node_type_info = create_stage()->get_node_type_info();
    if (node_type_info.display_name != orc::plugins::skeleton::kStageDisplayName ||
        node_type_info.type != orc::plugins::skeleton::kStageNodeType ||
        node_type_info.min_inputs != orc::plugins::skeleton::kStageMinInputs ||
        node_type_info.max_inputs != orc::plugins::skeleton::kStageMaxInputs ||
        node_type_info.min_outputs != orc::plugins::skeleton::kStageMinOutputs ||
        node_type_info.max_outputs != orc::plugins::skeleton::kStageMaxOutputs ||
        node_type_info.compatible_formats != orc::plugins::skeleton::kStageCompatibleFormats ||
        node_type_info.category() != orc::plugins::skeleton::kStageCategory) {
        if (error_message) {
            *error_message = "Stage metadata mismatch between plugin.h and NodeTypeInfo";
        }
        return false;
    }

    if (!register_stage(context, orc::plugins::skeleton::kStageName, &create_stage)) {
        if (error_message) {
            *error_message = "Failed to register stage from plugin metadata";
        }
        return false;
    }

    return true;
}
