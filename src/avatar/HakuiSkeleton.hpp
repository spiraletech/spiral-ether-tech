#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "avatar/AvatarAttachment.hpp"

class HakuiSkeleton {
public:
    HakuiSkeleton();
    ~HakuiSkeleton();

    HakuiSkeleton(HakuiSkeleton&&) noexcept;
    HakuiSkeleton& operator=(HakuiSkeleton&&) noexcept;

    HakuiSkeleton(const HakuiSkeleton&) = delete;
    HakuiSkeleton& operator=(const HakuiSkeleton&) = delete;

    bool buildDefaultHumanoid();
    std::size_t boneCount() const;
    bool ready() const;
    const std::vector<AvatarAttachment>& attachmentSlots() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
