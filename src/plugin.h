/*
 * File:        plugin.h
 * Module:      orc-stage-plugin-skeleton-passthrough
 * Purpose:     Plugin entrypoint metadata for SkeletonPassthroughStage
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 <Your Name>
 */

#pragma once

#include <orc/abi/orc_plugin_sdk.h>

#ifndef ORC_STAGE_PLUGIN_VERSION
#define ORC_STAGE_PLUGIN_VERSION "dev"
#endif

namespace orc::plugins::skeleton {

// Stage identifier used during plugin registration and project serialization.
// Allowed: non-empty stable identifier string (typically lowercase snake_case).
inline constexpr const char* kStageName = "skeleton_passthrough";

// Human-readable stage label shown in UI.
// Allowed: any non-empty display string.
inline constexpr const char* kStageDisplayName = "Skeleton Passthrough";

// Connectivity archetype for the stage.
// Allowed: SOURCE, SINK, TRANSFORM, MERGER, COMPLEX, ANALYSIS_SINK.
inline constexpr orc::NodeType kStageNodeType = NodeType::TRANSFORM;

// Minimum input ports this stage accepts.
// Allowed range: [0, UINT32_MAX], must satisfy min <= max.
inline constexpr uint32_t kStageMinInputs = 1;

// Maximum input ports this stage accepts.
// Allowed range: [0, UINT32_MAX], UINT32_MAX means "unbounded".
inline constexpr uint32_t kStageMaxInputs = 1;

// Minimum output ports this stage emits.
// Allowed range: [0, UINT32_MAX], must satisfy min <= max.
inline constexpr uint32_t kStageMinOutputs = 1;

// Maximum output ports this stage emits.
// Allowed range: [0, UINT32_MAX], UINT32_MAX means "unbounded".
inline constexpr uint32_t kStageMaxOutputs = 1;

// Video system compatibility gate.
// Allowed: ALL, NTSC_ONLY, PAL_ONLY.
inline constexpr orc::VideoFormatCompatibility kStageCompatibleFormats = VideoFormatCompatibility::ALL;

// Sink grouping for presenter/UI organization.
// Allowed: CORE, ANALYSIS, THIRD_PARTY.
inline constexpr orc::SinkCategory kStageSinkCategory = SinkCategory::THIRD_PARTY;

// UI menu category group shown in the node palette.
// Allowed: any non-empty display string.
inline constexpr const char* kStageMenuCategory = "Examples";

static_assert(kStageName[0] != '\0', "kStageName must not be empty");
static_assert(kStageDisplayName[0] != '\0', "kStageDisplayName must not be empty");
static_assert(kStageMenuCategory[0] != '\0', "kStageMenuCategory must not be empty");

static_assert(kStageMaxInputs >= kStageMinInputs, "kStageMaxInputs must be >= kStageMinInputs");
static_assert(kStageMaxOutputs >= kStageMinOutputs, "kStageMaxOutputs must be >= kStageMinOutputs");

// The ORC_STAGE_PLUGIN_DESCRIPTOR macro fills in the host ABI version, the
// plugin API version, and the toolchain tag from the SDK the plugin is
// compiled against — the descriptor fields a plugin author must never set by
// hand (the host rejects the plugin unless all three match it exactly).
inline constexpr orc::StagePluginDescriptor kPluginDescriptor =
    ORC_STAGE_PLUGIN_DESCRIPTOR(
        // Reverse-domain unique plugin ID.
        // Allowed: non-empty unique identifier string.
        "org.decodeorc.stage.skeleton_passthrough",

        // Plugin semantic version string provided by build system.
        // Allowed: non-empty version string (for example "1.2.3").
        ORC_STAGE_PLUGIN_VERSION,

        // SPDX license expression.
        // Allowed: valid SPDX expression string.
        "GPL-3.0-or-later",

        // Flags plugin provenance relative to host distribution.
        // Allowed: true (bundled/core plugin) or false (external/third-party).
        false);

} // namespace orc::plugins::skeleton
