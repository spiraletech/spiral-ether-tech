#include "social/ChatBubblePresentation.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace hakui::social {

namespace {

struct TextUnit {
    std::string bytes;
    float width = 0.0f;
    bool whitespace = false;
};

float smoothStep(float value) noexcept
{
    const float clamped = std::clamp(value, 0.0f, 1.0f);
    return clamped * clamped * (3.0f - 2.0f * clamped);
}

std::vector<TextUnit> textUnits(std::string_view text)
{
    std::vector<TextUnit> units;
    std::size_t offset = 0;
    while (offset < text.size()) {
        const unsigned char lead = static_cast<unsigned char>(text[offset]);
        std::size_t length = 1;
        if ((lead & 0xe0u) == 0xc0u) length = 2;
        else if ((lead & 0xf0u) == 0xe0u) length = 3;
        else if ((lead & 0xf8u) == 0xf0u) length = 4;
        length = std::min(length, text.size() - offset);
        TextUnit unit;
        unit.bytes = std::string{text.substr(offset, length)};
        unit.whitespace = length == 1 &&
            (text[offset] == ' ' || text[offset] == '\t' || text[offset] == '\n');
        unit.width = ChatBubblePresentation::glyphAdvanceUnits(
            length == 1 ? text[offset] : '?'
        );
        units.push_back(std::move(unit));
        offset += length;
    }
    return units;
}

float lineWidthUnits(std::string_view line) noexcept
{
    float width = 0.0f;
    for (const char character : line) {
        if ((static_cast<unsigned char>(character) & 0xc0u) == 0x80u) {
            continue;
        }
        width += ChatBubblePresentation::glyphAdvanceUnits(
            static_cast<unsigned char>(character) < 0x80u ? character : '?'
        );
    }
    return width;
}

} // namespace

BubblePresentationLayout ChatBubblePresentation::resolve(
    std::string_view text,
    float remainingSeconds,
    float totalSeconds,
    float distanceToCamera,
    const ChatBubbleMaterial& material,
    const BubblePresentationTuning& tuning
)
{
    BubblePresentationLayout result;
    if (text.empty() || remainingSeconds <= 0.0f || totalSeconds <= 0.0f) {
        return result;
    }

    const float maximumWidth = std::max(
        tuning.maximumBubbleWidth,
        material.padding * 2.0f + material.textScale * 12.0f
    );
    const float maximumTextUnits = std::max(
        12.0f,
        (maximumWidth - material.padding * 2.0f) / material.textScale
    );
    const std::size_t maximumLines = std::max<std::size_t>(1, tuning.maximumLines);
    const std::vector<TextUnit> units = textUnits(text);

    std::string line;
    float currentWidth = 0.0f;
    std::size_t lastBreakByte = std::string::npos;
    float widthAtBreak = 0.0f;
    std::size_t unitIndex = 0;
    while (unitIndex < units.size() && result.lines.size() < maximumLines) {
        const TextUnit& unit = units[unitIndex];
        if (unit.bytes == "\n") {
            result.lines.push_back(line);
            line.clear();
            currentWidth = 0.0f;
            lastBreakByte = std::string::npos;
            ++unitIndex;
            continue;
        }
        if (!line.empty() && currentWidth + unit.width > maximumTextUnits) {
            if (lastBreakByte != std::string::npos) {
                const std::string carry = line.substr(lastBreakByte + 1);
                line.resize(lastBreakByte);
                while (!line.empty() && line.back() == ' ') line.pop_back();
                result.lines.push_back(line);
                line = carry;
                currentWidth -= widthAtBreak;
            } else {
                result.lines.push_back(line);
                line.clear();
                currentWidth = 0.0f;
            }
            lastBreakByte = std::string::npos;
            if (result.lines.size() >= maximumLines) break;
            while (!line.empty() && line.front() == ' ') line.erase(line.begin());
            continue;
        }
        if (unit.whitespace && unit.bytes != "\t") {
            lastBreakByte = line.size();
            widthAtBreak = currentWidth + unit.width;
        }
        line += unit.bytes == "\t" ? " " : unit.bytes;
        currentWidth += unit.width;
        ++unitIndex;
    }
    if (result.lines.size() < maximumLines && !line.empty()) {
        while (!line.empty() && line.back() == ' ') line.pop_back();
        result.lines.push_back(line);
    }
    if (result.lines.empty()) {
        result.lines.emplace_back("...");
    }
    if (unitIndex < units.size()) {
        std::string& finalLine = result.lines.back();
        constexpr std::string_view ellipsis = "...";
        while (!finalLine.empty() &&
               lineWidthUnits(finalLine) + lineWidthUnits(ellipsis) >
                   maximumTextUnits) {
            finalLine.pop_back();
            while (!finalLine.empty() &&
                   (static_cast<unsigned char>(finalLine.back()) & 0xc0u) == 0x80u) {
                finalLine.pop_back();
            }
        }
        finalLine += ellipsis;
    }

    float longestLineUnits = 0.0f;
    for (const std::string& wrapped : result.lines) {
        longestLineUnits = std::max(longestLineUnits, lineWidthUnits(wrapped));
    }
    result.lineHeight = material.textScale * 9.0f;
    result.width = std::clamp(
        longestLineUnits * material.textScale + material.padding * 2.0f,
        0.92f,
        maximumWidth
    );
    result.height = static_cast<float>(result.lines.size()) * result.lineHeight +
        material.padding * 2.0f;

    const float distanceSpan = std::max(
        tuning.farDistance - tuning.nearDistance, 0.01f
    );
    const float distanceRatio = std::clamp(
        (distanceToCamera - tuning.nearDistance) / distanceSpan,
        0.0f,
        1.0f
    );
    result.scale = std::lerp(
        std::min(tuning.minimumScale, tuning.maximumScale),
        std::max(tuning.minimumScale, tuning.maximumScale),
        smoothStep(distanceRatio)
    );
    // Preserve the readable short-message scale while keeping wrapped bubbles
    // inside the camera frame. The clamp is deterministic and still respects
    // the same distance-derived scale range.
    const float wrappedLineCompression = 0.06f *
        static_cast<float>(result.lines.size() - 1);
    result.scale = std::max(
        std::min(tuning.minimumScale, tuning.maximumScale),
        result.scale - wrappedLineCompression
    );

    const float elapsedSeconds = std::max(0.0f, totalSeconds - remainingSeconds);
    const float fadeIn = std::max(tuning.fadeInSeconds, 0.01f);
    const float fadeOut = std::max(tuning.fadeOutSeconds, 0.01f);
    const float fadeInAlpha = smoothStep(elapsedSeconds / fadeIn);
    const float fadeOutAlpha = smoothStep(remainingSeconds / fadeOut);
    result.alpha = std::min(fadeInAlpha, fadeOutAlpha);
    result.phase = elapsedSeconds < fadeIn
        ? BubbleLifePhase::FadeIn
        : remainingSeconds <= fadeOut
            ? BubbleLifePhase::FadeOut
            : BubbleLifePhase::Hold;
    result.verticalOffset = tuning.reducedMotion
        ? 0.0f
        : -tuning.emergenceOffset * (1.0f - fadeInAlpha);
    result.visible = result.alpha > 0.001f;
    return result;
}

float ChatBubblePresentation::glyphAdvanceUnits(char character) noexcept
{
    switch (character) {
    case ' ':
    case '\t': return 3.0f;
    case 'i':
    case 'l':
    case 'I':
    case '!':
    case '.':
    case ',':
    case '\'':
    case ':': return 3.0f;
    case 'm':
    case 'w':
    case 'M':
    case 'W': return 6.0f;
    default: return 5.0f;
    }
}

std::string_view bubbleLifePhaseLabel(BubbleLifePhase phase) noexcept
{
    switch (phase) {
    case BubbleLifePhase::Hidden: return "Hidden";
    case BubbleLifePhase::FadeIn: return "FadeIn";
    case BubbleLifePhase::Hold: return "Hold";
    case BubbleLifePhase::FadeOut: return "FadeOut";
    }
    return "Hidden";
}

} // namespace hakui::social
