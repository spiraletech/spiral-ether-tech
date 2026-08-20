#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include "avatar/AvatarAttachment.hpp"

struct HakuiBoneDefinition {
    std::string name;
    int parent = -1;
};

// Hakui-owned rig definition.
//
// This class is deliberately engine/runtime agnostic: no SDL, Cal3D, Boost,
// renderer, or physics types are allowed here. Third-party skeletal runtimes
// translate this definition inside optional crystal backends.
class HakuiSkeleton {
public:
    bool buildDefaultHumanoid();

    std::size_t boneCount() const noexcept;
    bool ready() const noexcept;

    const std::vector<HakuiBoneDefinition>& bones() const noexcept;
    const std::vector<AvatarAttachment>& attachmentSlots() const noexcept;

    int findBone(std::string_view name) const noexcept;

private:
    int addBone(std::string name, int parent);

private:
    std::vector<HakuiBoneDefinition> bones_;
    std::vector<AvatarAttachment> attachments_;
};
