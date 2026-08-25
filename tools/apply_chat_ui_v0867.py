from pathlib import Path


def replace_exact(path: Path, old: str, new: str, label: str) -> None:
    text = path.read_text(encoding="utf-8")
    count = text.count(old)
    if count != 1:
        raise RuntimeError(
            f"{label}: expected exactly one source match in {path}, found {count}"
        )
    path.write_text(text.replace(old, new, 1), encoding="utf-8")
    print(f"[HAKUI v0.867] patched {label}: {path}")


renderer = Path("src/render/DebugWorldRenderer.cpp")
bubble = Path("src/social/ChatBubblePresentation.cpp")
chat_header = Path("src/social/ChatSystem.hpp")

# WORLD BUBBLE
# The layout resolver measures each line at 9 * textScale, but v0.861 rendered
# vertical cells at 1.18 * textScale. That made the real glyph block taller
# than the bubble that was measured for it. Render at the same cell size used
# by the layout model.
replace_exact(
    renderer,
    "            const float cellHeight = cellWidth * 1.18f;\n",
    "            const float cellHeight = cellWidth;\n",
    "world-bubble line-height parity",
)

old_world_text = '''            std::string wrappedText;\n            for (std::size_t line = 0; line < layout.lines.size(); ++line) {\n                if (line > 0) wrappedText.push_back('\\n');\n                wrappedText += fontDisplayText(layout.lines[line], 96);\n            }\n            const float cellWidth = scene.chatBubbleStyle.material.textScale *\n                presentationScale;\n            const float cellHeight = cellWidth;\n            const float textLeft = -panelWidth * 0.5f +\n                scene.chatBubbleStyle.material.padding * presentationScale;\n            const float textTop = tailSize + panelHeight -\n                scene.chatBubbleStyle.material.padding * presentationScale;\n            bindGlass(layout.alpha);\n            drawWorldText(\n                wrappedText,\n                multiply(\n                    bubbleRoot,\n                    translation({textLeft, textTop, -0.006f})\n                ),\n                cellWidth,\n                cellHeight,\n                0.040f,\n                96,\n                Shell\n            );\n            bindOpaque();\n'''

new_world_text = '''            const float cellWidth = scene.chatBubbleStyle.material.textScale *\n                presentationScale;\n            const float cellHeight = cellWidth;\n            const float textTop = tailSize + panelHeight -\n                scene.chatBubbleStyle.material.padding * presentationScale;\n\n            // The billboard basis used by this renderer presents local +X\n            // mirrored to the camera. Flip TEXT ONLY, then place each line's\n            // origin at +half its measured width so the resulting glyph block\n            // is centered around local X=0. This keeps the bubble body/tail\n            // untouched while making glyph shapes and reading order correct.\n            bindGlass(layout.alpha);\n            for (std::size_t lineIndex = 0; lineIndex < layout.lines.size(); ++lineIndex) {\n                const std::string displayLine =\n                    fontDisplayText(layout.lines[lineIndex], 96);\n                float lineUnits = 0.0f;\n                for (const char character : displayLine) {\n                    lineUnits += hakui::social::ChatBubblePresentation::\n                        glyphAdvanceUnits(character);\n                }\n                const float lineWidth = lineUnits * cellWidth;\n                const float lineY = textTop -\n                    static_cast<float>(lineIndex) *\n                    layout.lineHeight * presentationScale;\n                const Mat4 textRoot = multiply(\n                    bubbleRoot,\n                    multiply(\n                        translation({lineWidth * 0.5f, lineY, -0.006f}),\n                        scale({-1.0f, 1.0f, 1.0f})\n                    )\n                );\n                drawWorldText(\n                    displayLine,\n                    textRoot,\n                    cellWidth,\n                    cellHeight,\n                    0.040f,\n                    96,\n                    Shell\n                );\n            }\n            bindOpaque();\n'''
replace_exact(renderer, old_world_text, new_world_text, "centered readable world text")

# CHAT COMPOSER
# v0.861 measured horizontal glyph advance correctly but drew 7-row glyphs at
# 0.014 NDC per row inside a ~0.073 NDC inner panel. The text was physically
# taller than the field. Use a square-pixel-ish vertical cell and size the box
# from the actually displayed string. At the max width, scroll from the left
# while preserving the newest typed characters.
old_composer = '''    if (scene.chatInputActive) {\n        std::string entry = "say  ";\n        entry += fontDisplayText(scene.chatInputBuffer, 34);\n        entry += "_";\n        float entryUnits = 0.0f;\n        for (const char character : entry) {\n            entryUnits += hakui::social::ChatBubblePresentation::\n                glyphAdvanceUnits(character);\n        }\n        const float panelWidth = std::clamp(\n            entryUnits * 0.0044f + 0.18f, 0.54f, 1.28f\n        );\n        constexpr float panelHeight = 0.13f;\n        bindGlass(0.26f);\n        drawClipModel(\n            multiply(\n                translation({0.0f, -0.86f, 0.016f}),\n                scale({panelWidth, panelHeight * 0.66f, 0.004f})\n            ),\n            Cyan\n        );\n        bindGlass(0.48f);\n        drawClipModel(\n            multiply(\n                translation({0.0f, -0.86f, 0.012f}),\n                scale({panelWidth - 0.018f, panelHeight * 0.56f, 0.004f})\n            ),\n            Midnight\n        );\n        bindGlass(0.94f);\n        drawClipText(\n            entry,\n            -panelWidth * 0.5f + 0.075f,\n            -0.825f,\n            0.0044f,\n            0.014f,\n            Shell\n        );\n        bindOpaque();\n    }\n'''

new_composer = '''    if (scene.chatInputActive) {\n        constexpr float composerCellWidth = 0.0044f;\n        constexpr float composerCellHeight = 0.0085f;\n        constexpr float composerPaddingX = 0.075f;\n        constexpr float composerMinimumWidth = 0.54f;\n        constexpr float composerMaximumWidth = 1.70f;\n        constexpr float panelHeight = 0.13f;\n\n        const auto measureComposerUnits = [](std::string_view text) {\n            float units = 0.0f;\n            for (const char character : text) {\n                units += hakui::social::ChatBubblePresentation::\n                    glyphAdvanceUnits(character);\n            }\n            return units;\n        };\n\n        std::string visibleInput = fontDisplayText(scene.chatInputBuffer, 96);\n        std::string entry = "say  " + visibleInput + "_";\n        float entryUnits = measureComposerUnits(entry);\n        float desiredWidth =\n            entryUnits * composerCellWidth + composerPaddingX * 2.0f;\n\n        if (desiredWidth > composerMaximumWidth) {\n            constexpr std::string_view prefix = "say  ...";\n            while (!visibleInput.empty()) {\n                entry = std::string{prefix} + visibleInput + "_";\n                entryUnits = measureComposerUnits(entry);\n                desiredWidth =\n                    entryUnits * composerCellWidth + composerPaddingX * 2.0f;\n                if (desiredWidth <= composerMaximumWidth) {\n                    break;\n                }\n                visibleInput.erase(visibleInput.begin());\n            }\n        }\n\n        const float panelWidth = std::clamp(\n            desiredWidth, composerMinimumWidth, composerMaximumWidth\n        );\n        bindGlass(0.20f);\n        drawClipModel(\n            multiply(\n                translation({0.0f, -0.86f, 0.016f}),\n                scale({panelWidth, panelHeight * 0.66f, 0.004f})\n            ),\n            Cyan\n        );\n        bindGlass(0.40f);\n        drawClipModel(\n            multiply(\n                translation({0.0f, -0.86f, 0.012f}),\n                scale({panelWidth - 0.018f, panelHeight * 0.56f, 0.004f})\n            ),\n            Midnight\n        );\n        bindGlass(0.94f);\n        drawClipText(\n            entry,\n            -panelWidth * 0.5f + composerPaddingX,\n            -0.834f,\n            composerCellWidth,\n            composerCellHeight,\n            Shell\n        );\n        bindOpaque();\n    }\n'''
replace_exact(renderer, old_composer, new_composer, "adaptive chat composer")

# Compact the default bubble without sacrificing dynamic growth.
replace_exact(
    bubble,
    "        0.92f,\n        maximumWidth\n",
    "        0.72f,\n        maximumWidth\n",
    "short-message bubble minimum width",
)

# Softer IMVU-like glass defaults; sizing remains driven by measured text.
replace_exact(
    chat_header,
    "    float backgroundAlpha = 0.40f;\n    float borderAlpha = 0.50f;\n    float cornerRadius = 0.20f;\n    float padding = 0.15f;\n    float textScale = 0.040f;\n    float glowStrength = 0.06f;\n    float tailSize = 0.16f;\n",
    "    float backgroundAlpha = 0.34f;\n    float borderAlpha = 0.42f;\n    float cornerRadius = 0.20f;\n    float padding = 0.14f;\n    float textScale = 0.040f;\n    float glowStrength = 0.045f;\n    float tailSize = 0.12f;\n",
    "social glass material defaults",
)

print("[HAKUI v0.867] chat UI source patch complete")
