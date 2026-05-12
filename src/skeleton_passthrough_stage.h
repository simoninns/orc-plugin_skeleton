/*
 * File:        skeleton_passthrough_stage.h
 * Module:      orc-stage-plugin-skeleton-passthrough
 * Purpose:     Passthrough stage that forwards one input artifact unchanged
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 <Your Name>
 */

#pragma once

#include "orc_plugin_sdk_compat.h"

#if __has_include(<orc/plugin/orc_stage_preview.h>)
#include <orc/plugin/orc_stage_preview.h>
#elif __has_include(<orc_stage_preview.h>)
#include <orc_stage_preview.h>
#else
#error "Unable to locate decode-orc stage preview SDK header"
#endif

#if __has_include(<orc/plugin/orc_stage_runtime.h>)
#include <orc/plugin/orc_stage_runtime.h>
#elif __has_include(<orc_stage_runtime.h>)
#include <orc_stage_runtime.h>
#else
#error "Unable to locate decode-orc stage runtime SDK header"
#endif

namespace orc {
class VideoFieldRepresentation;
}

namespace orc::plugins::skeleton {

class SkeletonPassthroughStage : public DAGStage, public PreviewableStage {
public:
    std::string version() const override;
    NodeTypeInfo get_node_type_info() const override;

    std::vector<ArtifactPtr> execute(
        const std::vector<ArtifactPtr>& inputs,
        const std::map<std::string, ParameterValue>& parameters,
        ObservationContext& observation_context) override;

    bool supports_preview() const override;
    std::vector<PreviewOption> get_preview_options() const override;
    PreviewImage render_preview(
        const std::string& option_id,
        uint64_t index,
        PreviewNavigationHint hint = PreviewNavigationHint::Random) const override;

    size_t required_input_count() const override;
    size_t output_count() const override;

private:
    mutable std::shared_ptr<const VideoFieldRepresentation> cached_output_;
};

} // namespace orc::plugins::skeleton
