#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "social/ChatSystem.hpp"

namespace hakui::social {

enum class BubbleLifePhase : std::uint8_t {
    Hidden,
    FadeIn,
    Hold,
    FadeOut
};

struct BubblePresentationTuning {
    float maximumBubbleWidth = 3.00f;
    std::size_t maximumLines = 4;
    float minimumScale = 0.82f;
    float maximumScale = 1.24f;
    float nearDistance = 3.0f;
    float farDistance = 14.0f;
    float fadeInSeconds = 0.16f;
    float fadeOutSeconds = 0.42f;
    float emergenceOffset = 0.08f;
    bool reducedMotion = false;
};

struct BubblePresentationLayout {
    bool visible = false;
    BubbleLifePhase phase = BubbleLifePhase::Hidden;
    std::vector<std::string> lines;
    float alpha = 0.0f;
    float scale = 1.0f;
    float width = 0.0f;
    float height = 0.0f;
    float lineHeight = 0.0f;
    float verticalOffset = 0.0f;
};

class ChatBubblePresentation {
public:
    static BubblePresentationLayout resolve(
        std::string_view text,
        float remainingSeconds,
        float totalSeconds,
        float distanceToCamera,
        const ChatBubbleMaterial& material = {},
        const BubblePresentationTuning& tuning = {}
    );

    static float glyphAdvanceUnits(char character) noexcept;
};

std::string_view bubbleLifePhaseLabel(BubbleLifePhase phase) noexcept;

} // namespace hakui::social
