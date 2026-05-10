#include "skeleton_passthrough_stage.h"

#include <iostream>

int main()
{
    orc::plugins::skeleton::SkeletonPassthroughStage stage;

    const auto info = stage.get_node_type_info();
    if (info.stage_name != "skeleton_passthrough") {
        std::cerr << "Unexpected stage_name: " << info.stage_name << '\n';
        return 1;
    }

    if (info.type != orc::NodeType::TRANSFORM) {
        std::cerr << "Expected TRANSFORM node type\n";
        return 1;
    }

    if (stage.required_input_count() != 1 || stage.output_count() != 1) {
        std::cerr << "Expected one input and one output\n";
        return 1;
    }

    return 0;
}
