/*
 * File:        skeleton_passthrough_stage.cpp
 * Module:      orc-stage-plugin-skeleton-passthrough
 * Purpose:     Passthrough stage that forwards one input artifact unchanged
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 <Your Name>
 */

#include "skeleton_passthrough_stage.h"

#include <algorithm>

namespace orc::plugins::skeleton {

namespace {

uint8_t sample_to_byte(uint16_t sample)
{
    return static_cast<uint8_t>(sample >> 8);
}

const VideoFieldRepresentation::sample_type* get_channel_line(
    const VideoFieldRepresentation* rep,
    FieldID field_id, uint32_t line_index, bool want_chroma)
{
    if (!rep) return nullptr;
    if (!rep->has_separate_channels()) return rep->get_line(field_id, line_index);
    if (want_chroma) {
        const auto* ch = rep->get_line_chroma(field_id, line_index);
        return ch ? ch : rep->get_line(field_id, line_index);
    }
    const auto* luma = rep->get_line_luma(field_id, line_index);
    return luma ? luma : rep->get_line(field_id, line_index);
}

PreviewImage render_field_preview(
    const std::shared_ptr<const VideoFieldRepresentation>& rep, FieldID field_id)
{
    PreviewImage image{};
    if (!rep) return image;
    const auto desc = rep->get_descriptor(field_id);
    if (!desc.has_value() || desc->width == 0 || desc->height == 0) return image;
    image.width = static_cast<uint32_t>(desc->width);
    image.height = static_cast<uint32_t>(desc->height);
    image.rgb_data.resize(static_cast<size_t>(image.width) * image.height * 3, 0);
    for (uint32_t y = 0; y < image.height; ++y) {
        const auto* line = get_channel_line(rep.get(), field_id, y, false);
        if (!line) continue;
        for (uint32_t x = 0; x < image.width; ++x) {
            const auto v = sample_to_byte(line[x]);
            const auto off = (static_cast<size_t>(y) * image.width + x) * 3;
            image.rgb_data[off] = image.rgb_data[off + 1] = image.rgb_data[off + 2] = v;
        }
    }
    return image;
}

PreviewImage render_frame_preview(
    const std::shared_ptr<const VideoFieldRepresentation>& rep,
    uint64_t frame_index)
{
    PreviewImage image{};
    if (!rep) return image;
    const auto range = rep->field_range();
    if (!range.is_valid()) return image;
    const auto first_field = FieldID(range.start.value() + frame_index * 2);
    const auto second_field = FieldID(first_field.value() + 1);
    if (!rep->has_field(first_field) || !rep->has_field(second_field)) return image;
    const auto fd1 = rep->get_descriptor(first_field);
    const auto fd2 = rep->get_descriptor(second_field);
    if (!fd1.has_value() || !fd2.has_value()) return image;
    const auto width = static_cast<uint32_t>(std::max(fd1->width, fd2->width));
    const auto h1 = static_cast<uint32_t>(fd1->height);
    const auto h2 = static_cast<uint32_t>(fd2->height);
    const auto fh = std::max(h1, h2);
    if (width == 0 || fh == 0) return image;
    image.width = width;
    image.height = fh * 2;
    image.rgb_data.resize(static_cast<size_t>(image.width) * image.height * 3, 0);
    for (uint32_t y = 0; y < image.height; ++y) {
        const bool use_first = (y % 2) == 0;
        const uint32_t fl = y / 2;
        const auto sf = use_first ? first_field : second_field;
        const auto sh = use_first ? h1 : h2;
        if (fl >= sh) continue;
        const auto* line = get_channel_line(rep.get(), sf, fl, false);
        if (!line) continue;
        for (uint32_t x = 0; x < image.width; ++x) {
            const auto v = sample_to_byte(line[x]);
            const auto off = (static_cast<size_t>(y) * image.width + x) * 3;
            image.rgb_data[off] = image.rgb_data[off + 1] = image.rgb_data[off + 2] = v;
        }
    }
    return image;
}

} // namespace

std::string SkeletonPassthroughStage::version() const
{
    return "1.0.0";
}

NodeTypeInfo SkeletonPassthroughStage::get_node_type_info() const
{
    return NodeTypeInfo{
        NodeType::TRANSFORM, "skeleton_passthrough", "Skeleton Passthrough",
        "Minimal external plugin stage that forwards one input artifact.",
        1, 1, 1, 1, VideoFormatCompatibility::ALL, SinkCategory::THIRD_PARTY, "Examples"
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

    cached_output_ = std::dynamic_pointer_cast<const VideoFieldRepresentation>(inputs.at(0));
    return {inputs.at(0)};
}

bool SkeletonPassthroughStage::supports_preview() const
{
    return true;
}

std::vector<PreviewOption> SkeletonPassthroughStage::get_preview_options() const
{
    if (!cached_output_) return {};
    const auto range = cached_output_->field_range();
    if (!range.is_valid()) return {};
    const auto first_descriptor = cached_output_->get_descriptor(range.start);
    if (!first_descriptor.has_value() || first_descriptor->width == 0 || first_descriptor->height == 0) return {};
    const auto field_count = cached_output_->field_count();
    const auto w = static_cast<uint32_t>(first_descriptor->width);
    const auto h = static_cast<uint32_t>(first_descriptor->height);
    std::vector<PreviewOption> options;
    options.push_back(PreviewOption{"field",     "Field",     false, w, h,     field_count,     1.0});
    options.push_back(PreviewOption{"field_raw", "Field Raw", false, w, h,     field_count,     1.0});
    if (field_count >= 2) {
        options.push_back(PreviewOption{"split",     "Split",     false, w, h * 2, field_count / 2, 1.0});
        options.push_back(PreviewOption{"split_raw", "Split Raw", false, w, h * 2, field_count / 2, 1.0});
        options.push_back(PreviewOption{"frame",     "Frame",     false, w, h * 2, field_count / 2, 1.0});
        options.push_back(PreviewOption{"frame_raw", "Frame Raw", false, w, h * 2, field_count / 2, 1.0});
    }
    return options;
}

PreviewImage SkeletonPassthroughStage::render_preview(
    const std::string& option_id,
    uint64_t index,
    PreviewNavigationHint hint) const
{
    if (!cached_output_) return PreviewImage{};
    (void)hint;
    const auto range = cached_output_->field_range();
    if (!range.is_valid()) return PreviewImage{};
    if (option_id == "field" || option_id == "field_raw") {
        if (index >= cached_output_->field_count()) return PreviewImage{};
        const auto field_id = FieldID(range.start.value() + index);
        if (!cached_output_->has_field(field_id)) return PreviewImage{};
        return render_field_preview(cached_output_, field_id);
    }
    if (option_id == "split"     || option_id == "split_raw" ||
        option_id == "frame"     || option_id == "frame_raw") {
        const auto frame_count = cached_output_->field_count() / 2;
        if (index >= frame_count) return PreviewImage{};
        return render_frame_preview(cached_output_, index);
    }
    return PreviewImage{};
}

size_t SkeletonPassthroughStage::required_input_count() const
{
    return 1;
}

size_t SkeletonPassthroughStage::output_count() const
{
    return 1;
}

} // namespace orc::plugins::skeleton
