from pathlib import Path


def replace_exact(path: Path, old: str, new: str, label: str) -> None:
    text = path.read_text(encoding="utf-8")
    count = text.count(old)
    if count != 1:
        raise RuntimeError(
            f"{label}: expected exactly one source match in {path}, found {count}"
        )
    path.write_text(text.replace(old, new, 1), encoding="utf-8")
    print(f"[HAKUI v0.868] patched {label}: {path}")


def replace_all(path: Path, old: str, new: str, label: str) -> None:
    text = path.read_text(encoding="utf-8")
    count = text.count(old)
    if count == 0:
        raise RuntimeError(f"{label}: no source matches in {path}")
    path.write_text(text.replace(old, new), encoding="utf-8")
    print(f"[HAKUI v0.868] patched {label} ({count} matches): {path}")


renderer = Path("src/render/DebugWorldRenderer.cpp")
renderer_header = Path("src/render/DebugWorldRenderer.hpp")
app = Path("src/core/HakuiApp.cpp")

# Scene data: preserve ChatSystem as owner of history, but expose a read-only
# view plus local identity metadata to presentation. speakerId remains the
# network-facing stable seam for future multiplayer.
replace_exact(
    renderer_header,
    "#include <cstddef>\n#include <span>\n#include <string_view>\n",
    "#include <cstddef>\n#include <cstdint>\n#include <deque>\n#include <span>\n#include <string_view>\n",
    "renderer scene social includes",
)

replace_exact(
    renderer_header,
    "    bool chatInputActive = false;\n"
    "    std::string_view chatInputBuffer{};\n"
    "    bool chatBubbleActive = false;\n",
    "    bool chatInputActive = false;\n"
    "    std::string_view chatInputBuffer{};\n"
    "    const std::deque<hakui::social::ChatMessage>* chatHistory = nullptr;\n"
    "    std::uint32_t localSpeakerId = 1;\n"
    "    std::string_view localDisplayName{\"ETHER\"};\n"
    "    std::string_view localHandle{\"@local\"};\n"
    "    bool chatBubbleActive = false;\n",
    "scene chat history and identity seam",
)

replace_exact(
    app,
    "    scene.chatInputActive = chat_.inputActive();\n"
    "    scene.chatInputBuffer = chat_.inputBuffer();\n"
    "    scene.chatBubbleActive = chat_.bubble().active;\n",
    "    scene.chatInputActive = chat_.inputActive();\n"
    "    scene.chatInputBuffer = chat_.inputBuffer();\n"
    "    scene.chatHistory = &chat_.history();\n"
    "    scene.localSpeakerId = 1;\n"
    "    scene.localDisplayName = \"ETHER\";\n"
    "    scene.localHandle = \"@ether\";\n"
    "    scene.chatBubbleActive = chat_.bubble().active;\n",
    "app scene social identity/history bridge",
)

# World identity: an always-on IMVU-style nametag sits above the avatar. The
# speech bubble is lifted slightly so it occupies a separate visual layer.
old_bubble_start = '''    if (scene.chatBubbleActive && !scene.chatBubbleText.empty()) {\n        const float anchorY = player.y +\n            groundContact.visualRootAbovePlayerBase + 2.95f + idleBreath -\n            rideCompression;\n'''

new_bubble_start = '''    if (!scene.localDisplayName.empty()) {\n        const float tagAnchorY = player.y +\n            groundContact.visualRootAbovePlayerBase + 3.05f + idleBreath -\n            rideCompression;\n        const float tagDx = cameraEyeX_ - player.x;\n        const float tagDz = cameraEyeZ_ - player.z;\n        const float tagYaw = std::atan2(tagDx, tagDz);\n        const Mat4 tagRoot = multiply(\n            translation({player.x, tagAnchorY, player.z}),\n            rotationY(tagYaw)\n        );\n\n        const auto drawCenteredNametag = [&](std::string_view source,\n                                             float yOffset,\n                                             float cellSize,\n                                             Uint32 palette,\n                                             float alpha) {\n            const std::string display = fontDisplayText(source, 48);\n            float units = 0.0f;\n            for (const char character : display) {\n                units += hakui::social::ChatBubblePresentation::\n                    glyphAdvanceUnits(character);\n            }\n            const float lineWidth = units * cellSize;\n            bindGlass(alpha);\n            drawWorldText(\n                display,\n                multiply(\n                    tagRoot,\n                    multiply(\n                        translation({lineWidth * 0.5f, yOffset, -0.005f}),\n                        scale({-1.0f, 1.0f, 1.0f})\n                    )\n                ),\n                cellSize,\n                cellSize,\n                0.030f,\n                64,\n                palette\n            );\n        };\n\n        std::string identityLine{scene.localDisplayName};\n        identityLine += "  #";\n        identityLine += std::to_string(scene.localSpeakerId);\n        drawCenteredNametag(identityLine, 0.0f, 0.018f, Cyan, 0.92f);\n        if (!scene.localHandle.empty()) {\n            drawCenteredNametag(scene.localHandle, -0.16f, 0.014f, Shell, 0.72f);\n        }\n        bindOpaque();\n    }\n\n    if (scene.chatBubbleActive && !scene.chatBubbleText.empty()) {\n        const float anchorY = player.y +\n            groundContact.visualRootAbovePlayerBase + 3.38f + idleBreath -\n            rideCompression;\n'''
replace_exact(renderer, old_bubble_start, new_bubble_start, "IMVU world nametag layer")

# Persistent bottom-left local chat log. It intentionally consumes existing
# ChatMessage history and speakerId instead of inventing a second chat store.
composer_marker = '''    if (scene.chatInputActive) {\n        constexpr float composerCellWidth = 0.0044f;\n'''

chat_log = '''    if (scene.chatHistory && !scene.chatHistory->empty()) {\n        constexpr std::size_t maximumVisibleRows = 5;\n        constexpr float logCellWidth = 0.00355f;\n        constexpr float logCellHeight = 0.0069f;\n        constexpr float logWidth = 1.18f;\n        constexpr float logCenterX = -0.36f;\n        constexpr float logBottom = -0.72f;\n        constexpr float logRowHeight = 0.095f;\n        constexpr float logPadX = 0.055f;\n        constexpr float logPadY = 0.060f;\n\n        const auto measureLogUnits = [](std::string_view text) {\n            float units = 0.0f;\n            for (const char character : text) {\n                units += hakui::social::ChatBubblePresentation::\n                    glyphAdvanceUnits(character);\n            }\n            return units;\n        };\n        const auto fitLogLine = [&](std::string line) {\n            const float maximumUnits =\n                (logWidth - logPadX * 2.0f) / logCellWidth;\n            bool clipped = false;\n            while (!line.empty() && measureLogUnits(line) > maximumUnits) {\n                line.pop_back();\n                clipped = true;\n            }\n            if (clipped && line.size() > 3) {\n                while (line.size() > 3 &&\n                       measureLogUnits(line + "...") > maximumUnits) {\n                    line.pop_back();\n                }\n                line += "...";\n            }\n            return line;\n        };\n\n        const auto& history = *scene.chatHistory;\n        const std::size_t visibleRows = std::min(\n            maximumVisibleRows, history.size()\n        );\n        const float logHeight = logPadY * 2.0f +\n            0.085f + static_cast<float>(visibleRows) * logRowHeight;\n        const float logCenterY = logBottom + logHeight * 0.5f;\n\n        bindGlass(0.12f);\n        drawClipModel(\n            multiply(\n                translation({logCenterX, logCenterY, 0.010f}),\n                scale({logWidth + 0.018f, logHeight + 0.018f, 0.003f})\n            ),\n            Cyan\n        );\n        bindGlass(0.31f);\n        drawClipModel(\n            multiply(\n                translation({logCenterX, logCenterY, 0.008f}),\n                scale({logWidth, logHeight, 0.003f})\n            ),\n            Midnight\n        );\n\n        std::string header = "LOCAL CHAT // ";\n        header += std::string{scene.localDisplayName};\n        if (!scene.localHandle.empty()) {\n            header += " ";\n            header += std::string{scene.localHandle};\n        }\n        header += " #" + std::to_string(scene.localSpeakerId);\n        bindGlass(0.90f);\n        drawClipText(\n            fitLogLine(header),\n            logCenterX - logWidth * 0.5f + logPadX,\n            logBottom + logHeight - 0.055f,\n            logCellWidth,\n            logCellHeight,\n            Cyan\n        );\n\n        const std::size_t first = history.size() - visibleRows;\n        for (std::size_t row = 0; row < visibleRows; ++row) {\n            const hakui::social::ChatMessage& message = history[first + row];\n            std::string line;\n            Uint32 palette = Shell;\n            if (message.channel == hakui::social::ChatChannel::System) {\n                line = "[SYSTEM] HAKUI: ";\n                palette = Amber;\n            } else {\n                line = "[LOCAL] ";\n                if (message.speakerId == scene.localSpeakerId) {\n                    line += std::string{scene.localDisplayName};\n                } else {\n                    line += "USER";\n                }\n                line += "#" + std::to_string(message.speakerId) + ": ";\n                palette = message.speakerId == scene.localSpeakerId ? Cyan : Shell;\n            }\n            line += fontDisplayText(message.text, 64);\n            const float rowY = logBottom + logHeight - 0.145f -\n                static_cast<float>(row) * logRowHeight;\n            bindGlass(0.86f);\n            drawClipText(\n                fitLogLine(line),\n                logCenterX - logWidth * 0.5f + logPadX,\n                rowY,\n                logCellWidth,\n                logCellHeight,\n                palette\n            );\n        }\n        bindOpaque();\n    }\n\n    if (scene.chatInputActive) {\n        constexpr float composerCellWidth = 0.0044f;\n'''
replace_exact(renderer, composer_marker, chat_log, "persistent local chat log")

# Dock the v0.867 adaptive composer directly under the chat history panel.
replace_exact(
    renderer,
    "        constexpr float composerMaximumWidth = 1.70f;\n"
    "        constexpr float panelHeight = 0.13f;\n",
    "        constexpr float composerMaximumWidth = 1.18f;\n"
    "        constexpr float composerCenterX = -0.36f;\n"
    "        constexpr float panelHeight = 0.13f;\n",
    "composer dock width and anchor",
)
replace_exact(
    renderer,
    "                translation({0.0f, -0.86f, 0.016f}),\n",
    "                translation({composerCenterX, -0.86f, 0.016f}),\n",
    "composer cyan dock position",
)
replace_exact(
    renderer,
    "                translation({0.0f, -0.86f, 0.012f}),\n",
    "                translation({composerCenterX, -0.86f, 0.012f}),\n",
    "composer inner dock position",
)
replace_exact(
    renderer,
    "            -panelWidth * 0.5f + composerPaddingX,\n",
    "            composerCenterX - panelWidth * 0.5f + composerPaddingX,\n",
    "composer dock text origin",
)

# Visible/internal version identity. The v0.867 layout patch is still applied
# first by CI; v0.868 is the social presentation layer on top of that baseline.
replace_all(app, "v0.861-dev", "v0.868-dev", "app development version")
replace_all(app, "v0.861", "v0.868", "app social version")
replace_all(app, "SOCIAL BUBBLE VISUAL OVERHAUL", "IMVU SOCIAL STACK", "app social milestone label")
replace_all(renderer, "v0.861", "v0.868", "renderer version label")

print("[HAKUI v0.868] IMVU-PC social stack patch complete")
