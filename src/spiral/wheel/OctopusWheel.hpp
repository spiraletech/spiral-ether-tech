#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <utility>

#include "spiral/core/SpiralTypes.hpp"

namespace spiral {

struct MindPolicyDomain {};
struct CodingActionDomain {};

template <typename Domain>
class OctopusWheel {
public:
    struct Notch {
        std::size_t index = 0;
        std::string label;
    };

    OctopusWheel()
    {
        for (std::size_t i = 0; i < kOctopusNotchCount; ++i) {
            notches_[i].index = i;
        }
    }

    // Labels are supplied by the canon/config layer. We do not invent them in
    // engine code. The invariant is the eight-slot order and one active slot.
    bool setLabel(std::size_t index, std::string label)
    {
        if (index >= kOctopusNotchCount) {
            return false;
        }
        notches_[index].label = std::move(label);
        return true;
    }

    bool select(std::size_t index)
    {
        if (index >= kOctopusNotchCount) {
            return false;
        }
        active_ = index;
        return true;
    }

    void clear()
    {
        active_.reset();
    }

    std::optional<std::size_t> activeIndex() const noexcept
    {
        return active_;
    }

    const Notch* activeNotch() const noexcept
    {
        if (!active_) {
            return nullptr;
        }
        return &notches_[*active_];
    }

    const std::array<Notch, kOctopusNotchCount>& notches() const noexcept
    {
        return notches_;
    }

private:
    std::array<Notch, kOctopusNotchCount> notches_{};
    std::optional<std::size_t> active_;
};

// The types are intentionally orthogonal. A MindWheel is policy state only;
// a CodingWheel is action state only. They cannot directly call each other.
using MindWheel = OctopusWheel<MindPolicyDomain>;
using CodingWheel = OctopusWheel<CodingActionDomain>;

struct WheelSnapshot {
    std::optional<std::size_t> mindPolicy;
    std::optional<std::size_t> codingAction;
};

} // namespace spiral
