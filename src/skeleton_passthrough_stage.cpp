/*
 * File:        skeleton_passthrough_stage.cpp
 * Module:      orc-stage-plugin-skeleton-passthrough
 * Purpose:     Passthrough stage that forwards one input artifact unchanged
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 <Your Name>
 */

#include "skeleton_passthrough_stage.h"

namespace orc::plugins::skeleton {

std::string SkeletonPassthroughStage::version() const
{
    return "1.0.0";
}

NodeTypeInfo SkeletonPassthroughStage::get_node_type_info() const
{
    return NodeTypeInfo{
        NodeType::TRANSFORM, "skeleton_passthrough", "Skeleton Passthrough",
        "Minimal external plugin stage that forwards one input artifact.",
        1, 1, 1, 1, VideoFormatCompatibility::ALL
    };
}

std::vector<ArtifactPtr> SkeletonPassthroughStage::execute(
    const std::vector<ArtifactPtr>& inputs,
    const std::map<std::string, ParameterValue>&,
    ObservationContext&)
{
    if (inputs.empty()) {
        cached_output_.reset();
        return {};
    }

    // A passthrough forwards the artifact unchanged; caching it as a
    // VideoFrameRepresentation is what enables the preview capability below.
    // Stages that modify sample data instead wrap the input in a
    // VideoFrameRepresentationWrapper subclass and forward that.
    cached_output_ = std::dynamic_pointer_cast<const VideoFrameRepresentation>(inputs.at(0));
    return {inputs.at(0)};
}

size_t SkeletonPassthroughStage::required_input_count() const
{
    return 1;
}

size_t SkeletonPassthroughStage::output_count() const
{
    return 1;
}

StagePreviewCapability SkeletonPassthroughStage::get_preview_capability() const
{
    // Plugins linked against the installed SDK package can return
    // PreviewHelpers::make_signal_preview_capability(cached_output_) instead.
    // The capability is composed by hand here so this skeleton also builds in
    // the header-only in-tree CI configuration, which links no host libraries.
    StagePreviewCapability capability{};
    if (!cached_output_ || cached_output_->frame_count() == 0) {
        return capability;
    }

    const auto params = cached_output_->get_video_parameters();
    if (!params.has_value() || !params->is_valid()) {
        return capability;
    }

    const bool is_yc = cached_output_->has_separate_channels();
    VideoDataType data_type;
    if (params->system == VideoSystem::NTSC || params->system == VideoSystem::PAL_M) {
        data_type = is_yc ? VideoDataType::YC_NTSC : VideoDataType::CompositeNTSC;
    } else {
        data_type = is_yc ? VideoDataType::YC_PAL : VideoDataType::CompositePAL;
    }

    const auto active_width =
        params->active_video_end > params->active_video_start
            ? static_cast<uint32_t>(params->active_video_end - params->active_video_start)
            : static_cast<uint32_t>(params->frame_width_nominal);
    const auto active_height =
        params->last_active_frame_line > params->first_active_frame_line
            ? static_cast<uint32_t>(params->last_active_frame_line - params->first_active_frame_line)
            : static_cast<uint32_t>(params->frame_height);

    double dar_correction = 1.0;
    if (active_width > 0 && active_height > 0) {
        const double active_ratio =
            static_cast<double>(active_width) / static_cast<double>(active_height);
        dar_correction = (4.0 / 3.0) / active_ratio;
    }

    capability.supported_data_types = {data_type};
    capability.navigation_extent.item_count = cached_output_->frame_count();
    capability.navigation_extent.granularity = 1;
    capability.navigation_extent.item_label = "frame";
    capability.geometry.active_width = active_width;
    capability.geometry.active_height = active_height;
    capability.geometry.display_aspect_ratio = 4.0 / 3.0;
    capability.geometry.dar_correction_factor = dar_correction;
    return capability;
}

} // namespace orc::plugins::skeleton
