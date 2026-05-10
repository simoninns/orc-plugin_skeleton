#include "skeleton_passthrough_stage.h"

#include <algorithm>

namespace orc::plugins::skeleton {

namespace {

enum class PreviewChannel {
    Composite,
    Luma,
    Chroma,
    CompositeYC,
};

struct ParsedPreviewOption {
    std::string base_option_id;
    PreviewChannel channel;
};

ParsedPreviewOption parse_preview_option_id(const std::string& option_id)
{
    ParsedPreviewOption parsed{option_id, PreviewChannel::Composite};

    if (option_id.find("_yc") != std::string::npos) {
        parsed.channel = PreviewChannel::CompositeYC;
        const auto pos = option_id.find("_yc");
        parsed.base_option_id = option_id.substr(0, pos);
        if (pos + 3 < option_id.size() && option_id.substr(pos + 3) == "_raw") {
            parsed.base_option_id += "_raw";
        }
        return parsed;
    }

    if (option_id.find("_y") != std::string::npos) {
        parsed.channel = PreviewChannel::Luma;
        const auto pos = option_id.find("_y");
        parsed.base_option_id = option_id.substr(0, pos);
        if (pos + 2 < option_id.size() && option_id.substr(pos + 2) == "_raw") {
            parsed.base_option_id += "_raw";
        }
        return parsed;
    }

    if (option_id.find("_c") != std::string::npos) {
        parsed.channel = PreviewChannel::Chroma;
        const auto pos = option_id.find("_c");
        parsed.base_option_id = option_id.substr(0, pos);
        if (pos + 2 < option_id.size() && option_id.substr(pos + 2) == "_raw") {
            parsed.base_option_id += "_raw";
        }
        return parsed;
    }

    return parsed;
}

const VideoFieldRepresentation::sample_type* get_channel_line(
    const VideoFieldRepresentation* representation,
    FieldID field_id,
    uint32_t line_index,
    PreviewChannel channel)
{
    if (!representation) {
        return nullptr;
    }

    if (!representation->has_separate_channels()) {
        return representation->get_line(field_id, line_index);
    }

    if (channel == PreviewChannel::Chroma) {
        const auto* chroma = representation->get_line_chroma(field_id, line_index);
        if (chroma) {
            return chroma;
        }
        return representation->get_line(field_id, line_index);
    }

    const auto* luma = representation->get_line_luma(field_id, line_index);
    if (luma) {
        return luma;
    }
    return representation->get_line(field_id, line_index);
}

uint16_t sample_for_channel(
    const VideoFieldRepresentation* representation,
    FieldID field_id,
    uint32_t line_index,
    uint32_t x,
    PreviewChannel channel)
{
    const auto* primary = get_channel_line(representation, field_id, line_index, channel);
    if (!primary) {
        return 0;
    }

    if (channel == PreviewChannel::CompositeYC && representation && representation->has_separate_channels()) {
        const auto* luma = representation->get_line_luma(field_id, line_index);
        const auto* chroma = representation->get_line_chroma(field_id, line_index);
        if (luma && chroma) {
            return static_cast<uint16_t>((static_cast<uint32_t>(luma[x]) + static_cast<uint32_t>(chroma[x])) / 2);
        }
    }

    return primary[x];
}

uint8_t sample_to_byte(uint16_t sample)
{
    return static_cast<uint8_t>(sample >> 8);
}

PreviewImage render_field_preview(
    const std::shared_ptr<const VideoFieldRepresentation>& representation,
    FieldID field_id,
    PreviewChannel channel)
{
    PreviewImage image{};
    if (!representation) {
        return image;
    }

    const auto descriptor = representation->get_descriptor(field_id);
    if (!descriptor.has_value() || descriptor->width == 0 || descriptor->height == 0) {
        return image;
    }

    image.width = static_cast<uint32_t>(descriptor->width);
    image.height = static_cast<uint32_t>(descriptor->height);
    image.rgb_data.resize(static_cast<size_t>(image.width) * static_cast<size_t>(image.height) * 3, 0);

    for (uint32_t y = 0; y < image.height; ++y) {
        const auto* line = get_channel_line(representation.get(), field_id, y, channel);
        if (!line) {
            continue;
        }

        for (uint32_t x = 0; x < image.width; ++x) {
            const auto luma = sample_to_byte(sample_for_channel(representation.get(), field_id, y, x, channel));
            const auto offset = (static_cast<size_t>(y) * image.width + x) * 3;
            image.rgb_data[offset] = luma;
            image.rgb_data[offset + 1] = luma;
            image.rgb_data[offset + 2] = luma;
        }
    }

    return image;
}

PreviewImage render_frame_preview(
    const std::shared_ptr<const VideoFieldRepresentation>& representation,
    FieldID first_field,
    FieldID second_field,
    PreviewChannel channel)
{
    PreviewImage image{};
    if (!representation) {
        return image;
    }

    const auto first_desc = representation->get_descriptor(first_field);
    const auto second_desc = representation->get_descriptor(second_field);
    if (!first_desc.has_value() || !second_desc.has_value()) {
        return image;
    }

    const auto width = static_cast<uint32_t>(std::max(first_desc->width, second_desc->width));
    const auto first_height = static_cast<uint32_t>(first_desc->height);
    const auto second_height = static_cast<uint32_t>(second_desc->height);
    const auto field_height = std::max(first_height, second_height);
    if (width == 0 || field_height == 0) {
        return image;
    }

    image.width = width;
    image.height = field_height * 2;
    image.rgb_data.resize(static_cast<size_t>(image.width) * static_cast<size_t>(image.height) * 3, 0);

    for (uint32_t y = 0; y < image.height; ++y) {
        const bool use_first = (y % 2) == 0;
        const uint32_t field_line = y / 2;
        const auto source_field = use_first ? first_field : second_field;
        const auto source_height = use_first ? first_height : second_height;

        if (field_line >= source_height) {
            continue;
        }

        const auto* line = get_channel_line(representation.get(), source_field, field_line, channel);
        if (!line) {
            continue;
        }

        for (uint32_t x = 0; x < image.width; ++x) {
            const auto luma = sample_to_byte(sample_for_channel(representation.get(), source_field, field_line, x, channel));
            const auto offset = (static_cast<size_t>(y) * image.width + x) * 3;
            image.rgb_data[offset] = luma;
            image.rgb_data[offset + 1] = luma;
            image.rgb_data[offset + 2] = luma;
        }
    }

    return image;
}

NodeTypeInfo make_node_type_info()
{
    return NodeTypeInfo(
        NodeType::TRANSFORM,
        "skeleton_passthrough",
        "Skeleton Passthrough",
        "Minimal external plugin stage that forwards one input artifact.",
        1,
        1,
        1,
        1,
        VideoFormatCompatibility::ALL,
        SinkCategory::THIRD_PARTY,
        "Examples");
}

} // namespace

std::string SkeletonPassthroughStage::version() const
{
    return "1.0.0";
}

NodeTypeInfo SkeletonPassthroughStage::get_node_type_info() const
{
    return make_node_type_info();
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
    if (!cached_output_) {
        return {};
    }

    const auto range = cached_output_->field_range();
    if (!range.is_valid()) {
        return {};
    }

    const auto first_descriptor = cached_output_->get_descriptor(range.start);
    if (!first_descriptor.has_value() || first_descriptor->width == 0 || first_descriptor->height == 0) {
        return {};
    }

    std::vector<PreviewOption> options;
    const auto field_count = cached_output_->field_count();

    options.push_back(PreviewOption{
        "field",
        "Field",
        false,
        static_cast<uint32_t>(first_descriptor->width),
        static_cast<uint32_t>(first_descriptor->height),
        field_count,
        1.0,
    });

    if (field_count >= 2) {
        options.push_back(PreviewOption{
            "frame",
            "Frame",
            false,
            static_cast<uint32_t>(first_descriptor->width),
            static_cast<uint32_t>(first_descriptor->height * 2),
            field_count / 2,
            1.0,
        });
    }

    return options;
}

PreviewImage SkeletonPassthroughStage::render_preview(
    const std::string& option_id,
    uint64_t index,
    PreviewNavigationHint hint) const
{
    if (!cached_output_) {
        return PreviewImage{};
    }

    (void)hint;
    const auto parsed_option = parse_preview_option_id(option_id);

    const auto range = cached_output_->field_range();
    if (!range.is_valid()) {
        return PreviewImage{};
    }

    if (parsed_option.base_option_id == "field" || parsed_option.base_option_id == "field_raw") {
        if (index >= cached_output_->field_count()) {
            return PreviewImage{};
        }
        const auto field_id = FieldID(range.start.value() + index);
        if (!cached_output_->has_field(field_id)) {
            return PreviewImage{};
        }
        return render_field_preview(cached_output_, field_id, parsed_option.channel);
    }

    if (parsed_option.base_option_id == "frame" || parsed_option.base_option_id == "frame_raw") {
        const auto frame_count = cached_output_->field_count() / 2;
        if (index >= frame_count) {
            return PreviewImage{};
        }

        const auto first_field = FieldID(range.start.value() + (index * 2));
        const auto second_field = FieldID(first_field.value() + 1);
        if (!cached_output_->has_field(first_field) || !cached_output_->has_field(second_field)) {
            return PreviewImage{};
        }
        return render_frame_preview(cached_output_, first_field, second_field, parsed_option.channel);
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
