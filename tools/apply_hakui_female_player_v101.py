from pathlib import Path


def replace_exact(path: Path, old: str, new: str, label: str) -> None:
    text = path.read_text(encoding="utf-8")
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{label}: expected exactly one match in {path}, found {count}")
    path.write_text(text.replace(old, new, 1), encoding="utf-8")
    print(f"[HAKUI FEMALE v1.01] patched {label}: {path}")


def replace_all(path: Path, old: str, new: str, label: str) -> None:
    text = path.read_text(encoding="utf-8")
    count = text.count(old)
    if count == 0:
        raise RuntimeError(f"{label}: no matches in {path}")
    path.write_text(text.replace(old, new), encoding="utf-8")
    print(f"[HAKUI FEMALE v1.01] patched {label} ({count} matches): {path}")


header = Path("src/render/DebugWorldRenderer.hpp")
renderer = Path("src/render/DebugWorldRenderer.cpp")
app = Path("src/core/HakuiApp.cpp")
cmake = Path("CMakeLists.txt")

# -----------------------------------------------------------------------------
# Runtime seam: the copied HAKUI build marks only its player presentation as
# female. It does NOT enter mannequin-lab mode, so world/gameplay behavior is
# unchanged: locomotion, ride physics, combat, seats, chat and interactions all
# remain authoritative in the original HAKUI systems.
# -----------------------------------------------------------------------------
replace_exact(
    header,
    "    bool mannequinLab = false;\n"
    "    bool mannequinFemale = false;\n"
    "    bool mannequinShowJoints = false;\n",
    "    bool mannequinLab = false;\n"
    "    bool mannequinFemale = false;\n"
    "    bool playerFemaleShell = false;\n"
    "    bool mannequinShowJoints = false;\n",
    "female player scene flag",
)

replace_exact(
    app,
    "namespace {\n\nconst char* combatStateLabel",
    "namespace {\n\n"
    "#if defined(HAKUI_FEMALE_PLAYER)\n"
    "constexpr bool kHakuiFemalePlayer = true;\n"
    "#else\n"
    "constexpr bool kHakuiFemalePlayer = false;\n"
    "#endif\n\n"
    "const char* combatStateLabel",
    "female player compile identity",
)

replace_exact(
    app,
    "    HakuiSceneState scene;\n"
    "    scene.paused = paused_;\n",
    "    HakuiSceneState scene;\n"
    "    scene.playerFemaleShell = kHakuiFemalePlayer;\n"
    "    scene.paused = paused_;\n",
    "activate female shell only in copied HAKUI build",
)

# Make the copied build visually identifiable without touching mechanics.
replace_exact(
    app,
    "    SDL_Log(\"[HAKUI] WORLD ONLINE\");\n",
    "    SDL_Log(kHakuiFemalePlayer\n"
    "        ? \"[HAKUI FEMALE] WORLD ONLINE // female shell // ghost rig authoritative\"\n"
    "        : \"[HAKUI] WORLD ONLINE\");\n",
    "female boot marker",
)

# -----------------------------------------------------------------------------
# Presentation promotion: reuse the validated female mannequin v0.1 dimensions
# inside normal HAKUI rendering, while keeping normal gameplay pose/contact
# targets. We selectively promote geometry-only branches; lab-only arm/foot
# pose presets, lab UI and lab world isolation remain gated by mannequinLab.
# -----------------------------------------------------------------------------

# Shoulder anchor.
replace_exact(
    renderer,
    "            scene.mannequinLab\n"
    "                ? (scene.mannequinFemale\n"
    "                    ? Vec3{side * 0.46f, 2.02f, 0.0f}\n"
    "                    : Vec3{side * 0.52f, 2.04f, 0.0f})\n"
    "                : Vec3{side * 0.60f, 2.12f, 0.0f},\n",
    "            scene.playerFemaleShell\n"
    "                ? Vec3{side * 0.46f, 2.02f, 0.0f}\n"
    "                : (scene.mannequinLab\n"
    "                    ? (scene.mannequinFemale\n"
    "                        ? Vec3{side * 0.46f, 2.02f, 0.0f}\n"
    "                        : Vec3{side * 0.52f, 2.04f, 0.0f})\n"
    "                    : Vec3{side * 0.60f, 2.12f, 0.0f}),\n",
    "female HAKUI shoulder anchor",
)

replace_exact(
    renderer,
    "        const float elbowOut = scene.mannequinLab\n"
    "            ? (scene.mannequinFemale ? 0.060f : 0.075f)\n"
    "            : 0.12f;\n",
    "        const float elbowOut = scene.playerFemaleShell\n"
    "            ? 0.060f\n"
    "            : (scene.mannequinLab\n"
    "                ? (scene.mannequinFemale ? 0.060f : 0.075f)\n"
    "                : 0.12f);\n",
    "female HAKUI elbow line",
)

replace_exact(
    renderer,
    "            scene.mannequinLab\n"
    "                ? (scene.mannequinFemale ? 0.132f : 0.145f)\n"
    "                : 0.23f\n",
    "            scene.playerFemaleShell\n"
    "                ? 0.132f\n"
    "                : (scene.mannequinLab\n"
    "                    ? (scene.mannequinFemale ? 0.132f : 0.145f)\n"
    "                    : 0.23f)\n",
    "female HAKUI upper arm width",
)
replace_exact(
    renderer,
    "            scene.mannequinLab\n"
    "                ? (scene.mannequinFemale ? 0.094f : 0.112f)\n"
    "                : 0.21f\n",
    "            scene.playerFemaleShell\n"
    "                ? 0.094f\n"
    "                : (scene.mannequinLab\n"
    "                    ? (scene.mannequinFemale ? 0.094f : 0.112f)\n"
    "                    : 0.21f)\n",
    "female HAKUI forearm width",
)

replace_exact(
    renderer,
    "            scene.mannequinLab\n"
    "                ? (scene.mannequinFemale\n"
    "                    ? Vec3{0.115f, 0.095f, 0.135f}\n"
    "                    : Vec3{0.135f, 0.115f, 0.150f})\n"
    "                : Vec3{0.24f, 0.20f, 0.24f},\n",
    "            scene.playerFemaleShell\n"
    "                ? Vec3{0.115f, 0.095f, 0.135f}\n"
    "                : (scene.mannequinLab\n"
    "                    ? (scene.mannequinFemale\n"
    "                        ? Vec3{0.115f, 0.095f, 0.135f}\n"
    "                        : Vec3{0.135f, 0.115f, 0.150f})\n"
    "                    : Vec3{0.24f, 0.20f, 0.24f}),\n",
    "female HAKUI hand dimensions",
)

# Legs keep normal gameplay contact targets, only shell widths/feet change.
replace_exact(
    renderer,
    "            scene.mannequinLab\n"
    "                ? (scene.mannequinFemale ? 0.205f : 0.188f)\n"
    "                : 0.27f\n",
    "            scene.playerFemaleShell\n"
    "                ? 0.205f\n"
    "                : (scene.mannequinLab\n"
    "                    ? (scene.mannequinFemale ? 0.205f : 0.188f)\n"
    "                    : 0.27f)\n",
    "female HAKUI thigh width",
)
replace_exact(
    renderer,
    "            scene.mannequinLab\n"
    "                ? (scene.mannequinFemale ? 0.132f : 0.145f)\n"
    "                : 0.25f\n",
    "            scene.playerFemaleShell\n"
    "                ? 0.132f\n"
    "                : (scene.mannequinLab\n"
    "                    ? (scene.mannequinFemale ? 0.132f : 0.145f)\n"
    "                    : 0.25f)\n",
    "female HAKUI calf width",
)
replace_exact(
    renderer,
    "            scene.mannequinLab\n"
    "                ? (scene.mannequinFemale\n"
    "                    ? Vec3{0.185f, 0.082f, 0.330f}\n"
    "                    : Vec3{0.205f, 0.095f, 0.360f})\n"
    "                : Vec3{0.32f, 0.14f, 0.50f},\n",
    "            scene.playerFemaleShell\n"
    "                ? Vec3{0.185f, 0.082f, 0.330f}\n"
    "                : (scene.mannequinLab\n"
    "                    ? (scene.mannequinFemale\n"
    "                        ? Vec3{0.185f, 0.082f, 0.330f}\n"
    "                        : Vec3{0.205f, 0.095f, 0.360f})\n"
    "                    : Vec3{0.32f, 0.14f, 0.50f}),\n",
    "female HAKUI foot dimensions",
)

# Pelvis shell.
replace_exact(
    renderer,
    "        scene.mannequinLab\n"
    "            ? (scene.mannequinFemale\n"
    "                ? Vec3{0.64f, 0.22f, 0.36f}\n"
    "                : Vec3{0.54f, 0.20f, 0.32f})\n"
    "            : Vec3{0.74f, 0.30f, 0.44f},\n",
    "        scene.playerFemaleShell\n"
    "            ? Vec3{0.64f, 0.22f, 0.36f}\n"
    "            : (scene.mannequinLab\n"
    "                ? (scene.mannequinFemale\n"
    "                    ? Vec3{0.64f, 0.22f, 0.36f}\n"
    "                    : Vec3{0.54f, 0.20f, 0.32f})\n"
    "                : Vec3{0.74f, 0.30f, 0.44f}),\n",
    "female HAKUI pelvis shell",
)

# Waist bridge is presentation-only, so it can be rendered for the female HAKUI
# shell without activating any mannequin-lab behavior.
replace_exact(
    renderer,
    "    if (scene.mannequinLab) {\n"
    "        // Upper pelvis/waist bridge: stacked geometry gives the lab body\n",
    "    if (scene.mannequinLab || scene.playerFemaleShell) {\n"
    "        // Upper pelvis/waist bridge: stacked geometry gives the human shell\n",
    "female HAKUI waist bridge gate",
)
replace_exact(
    renderer,
    "            scene.mannequinFemale\n"
    "                ? Vec3{0.40f, 0.13f, 0.29f}\n"
    "                : Vec3{0.46f, 0.13f, 0.30f},\n",
    "            (scene.mannequinFemale || scene.playerFemaleShell)\n"
    "                ? Vec3{0.40f, 0.13f, 0.29f}\n"
    "                : Vec3{0.46f, 0.13f, 0.30f},\n",
    "female HAKUI waist bridge dimensions",
)

# Torso transform frame uses the female lab profile in the copied game build.
replace_exact(
    renderer,
    "                            scene.mannequinLab\n"
    "                                ? (scene.mannequinFemale ? 0.54f : 0.58f)\n"
    "                                : 0.92f,\n"
    "                            scene.mannequinLab\n"
    "                                ? (scene.mannequinFemale ? 0.84f : 0.86f)\n"
    "                                : 0.94f,\n"
    "                            scene.mannequinLab\n"
    "                                ? (scene.mannequinFemale ? 0.30f : 0.31f)\n"
    "                                : 0.48f\n",
    "                            scene.playerFemaleShell\n"
    "                                ? 0.54f\n"
    "                                : (scene.mannequinLab\n"
    "                                    ? (scene.mannequinFemale ? 0.54f : 0.58f)\n"
    "                                    : 0.92f),\n"
    "                            scene.playerFemaleShell\n"
    "                                ? 0.84f\n"
    "                                : (scene.mannequinLab\n"
    "                                    ? (scene.mannequinFemale ? 0.84f : 0.86f)\n"
    "                                    : 0.94f),\n"
    "                            scene.playerFemaleShell\n"
    "                                ? 0.30f\n"
    "                                : (scene.mannequinLab\n"
    "                                    ? (scene.mannequinFemale ? 0.30f : 0.31f)\n"
    "                                    : 0.48f)\n",
    "female HAKUI torso frame",
)

replace_exact(
    renderer,
    "    if (scene.mannequinLab) {\n"
    "        drawModel(\n"
    "            multiply(\n"
    "                torso,\n",
    "    if (scene.mannequinLab || scene.playerFemaleShell) {\n"
    "        drawModel(\n"
    "            multiply(\n"
    "                torso,\n",
    "female HAKUI tapered torso gate",
)
replace_all(
    renderer,
    "scene.mannequinFemale ? 1.16f : 1.24f",
    "(scene.mannequinFemale || scene.playerFemaleShell) ? 1.16f : 1.24f",
    "female HAKUI ribcage width",
)
replace_all(
    renderer,
    "scene.mannequinFemale ? 1.08f : 1.10f",
    "(scene.mannequinFemale || scene.playerFemaleShell) ? 1.08f : 1.10f",
    "female HAKUI ribcage depth",
)
replace_all(
    renderer,
    "scene.mannequinFemale ? 0.72f : 0.86f",
    "(scene.mannequinFemale || scene.playerFemaleShell) ? 0.72f : 0.86f",
    "female HAKUI waist taper",
)
replace_all(
    renderer,
    "scene.mannequinFemale ? 0.92f : 0.96f",
    "(scene.mannequinFemale || scene.playerFemaleShell) ? 0.92f : 0.96f",
    "female HAKUI waist depth",
)

# Clavicle presentation can be shared; gameplay arm targets stay normal.
replace_exact(
    renderer,
    "    if (scene.mannequinLab) {\n"
    "        const float torsoYaw = scene.rideable.body.torsoYawRelativeToBoard;\n"
    "        const Vec3 clavicleCenter",
    "    if (scene.mannequinLab || scene.playerFemaleShell) {\n"
    "        const float torsoYaw = scene.rideable.body.torsoYawRelativeToBoard;\n"
    "        const Vec3 clavicleCenter",
    "female HAKUI clavicle gate",
)
replace_all(
    renderer,
    "scene.mannequinFemale ? 0.46f : 0.52f",
    "(scene.mannequinFemale || scene.playerFemaleShell) ? 0.46f : 0.52f",
    "female HAKUI clavicle spread",
)
replace_all(
    renderer,
    "scene.mannequinFemale ? 2.02f : 2.04f",
    "(scene.mannequinFemale || scene.playerFemaleShell) ? 2.02f : 2.04f",
    "female HAKUI clavicle height",
)
replace_all(
    renderer,
    "scene.mannequinFemale ? 0.082f : 0.095f",
    "(scene.mannequinFemale || scene.playerFemaleShell) ? 0.082f : 0.095f",
    "female HAKUI clavicle thickness",
)

# Neck/head shell.
replace_exact(
    renderer,
    "        scene.mannequinLab\n"
    "            ? (scene.mannequinFemale\n"
    "                ? Vec3{0.125f, 0.190f, 0.125f}\n"
    "                : Vec3{0.145f, 0.205f, 0.145f})\n"
    "            : Vec3{0.22f, 0.18f, 0.22f},\n",
    "        scene.playerFemaleShell\n"
    "            ? Vec3{0.125f, 0.190f, 0.125f}\n"
    "            : (scene.mannequinLab\n"
    "                ? (scene.mannequinFemale\n"
    "                    ? Vec3{0.125f, 0.190f, 0.125f}\n"
    "                    : Vec3{0.145f, 0.205f, 0.145f})\n"
    "                : Vec3{0.22f, 0.18f, 0.22f}),\n",
    "female HAKUI neck shell",
)
replace_exact(
    renderer,
    "        scene.mannequinLab\n"
    "            ? (scene.mannequinFemale\n"
    "                ? Vec3{0.39f, 0.47f, 0.37f}\n"
    "                : Vec3{0.42f, 0.50f, 0.40f})\n"
    "            : Vec3{0.56f, 0.58f, 0.52f},\n",
    "        scene.playerFemaleShell\n"
    "            ? Vec3{0.39f, 0.47f, 0.37f}\n"
    "            : (scene.mannequinLab\n"
    "                ? (scene.mannequinFemale\n"
    "                    ? Vec3{0.39f, 0.47f, 0.37f}\n"
    "                    : Vec3{0.42f, 0.50f, 0.40f})\n"
    "                : Vec3{0.56f, 0.58f, 0.52f}),\n",
    "female HAKUI head shell",
)

# -----------------------------------------------------------------------------
# Sibling executable: same HAKUI sources, same systems, compile-time female shell.
# The original `hakui` target is untouched and continues producing the polished
# baseline executable.
# -----------------------------------------------------------------------------
hakui_sources = '''        src/main.cpp\n        src/core/HakuiApp.cpp\n        src/core/HakuiApp.hpp\n        src/observer/NativeFrameCapture.cpp\n        src/observer/NativeFrameCapture.hpp\n        src/input/SdlInputBridge.cpp\n        src/input/SdlInputBridge.hpp\n        src/audio/HakuiAudio.cpp\n        src/audio/HakuiAudio.hpp\n        src/avatar/HakuiSkeleton.hpp\n        src/avatar/AvatarAttachment.hpp\n        src/render/DebugWorldRenderer.cpp\n        src/render/DebugWorldRenderer.hpp\n        src/render/Math3D.hpp\n        src/player/PlayerState.hpp\n        src/world/WorldState.hpp\n        src/systems/LocomotionRouter.hpp\n        src/systems/Interactable.hpp\n'''

anchor = (
    "    target_compile_definitions(hakui PRIVATE\n"
    "        HAKUI_GIT_SHA=\"${HAKUI_GIT_SHA}\"\n"
    "        HAKUI_GIT_BRANCH=\"${HAKUI_GIT_BRANCH}\"\n"
    "        HAKUI_BUILD_TIMESTAMP=\"${HAKUI_BUILD_TIMESTAMP}\"\n"
    "        HAKUI_BUILD_CONFIGURATION=\"${HAKUI_BUILD_CONFIGURATION}\"\n"
    "    )\n\n"
)
insert = anchor + (
    "    # ------------------------------------------------------------\n"
    "    # HAKUI FEMALE PLAYER COPY\n"
    "    # ------------------------------------------------------------\n"
    "    add_executable(hakui_female\n" + hakui_sources + "    )\n\n"
    "    set_target_properties(hakui_female PROPERTIES\n"
    "        OUTPUT_NAME \"SPIRAL-OS-HAKUI-FEMALE\"\n"
    "    )\n\n"
    "    target_link_libraries(hakui_female\n"
    "        PRIVATE\n"
    "            SDL3::SDL3\n"
    "            spiral_core\n"
    "            hakui_avatar_rig\n"
    "            hakui_interaction\n"
    "            hakui_gameplay\n"
    "            hakui_input\n"
    "            hakui_social\n"
    "            hakui_observer\n"
    "            hakui_camera\n"
    "            hakui_combat\n"
    "            hakui_tabletop\n"
    "    )\n\n"
    "    target_include_directories(hakui_female PRIVATE\n"
    "        src\n"
    "        \"${sdl3_SOURCE_DIR}/test\"\n"
    "    )\n\n"
    "    target_compile_definitions(hakui_female PRIVATE\n"
    "        HAKUI_FEMALE_PLAYER=1\n"
    "        HAKUI_GIT_SHA=\"${HAKUI_GIT_SHA}\"\n"
    "        HAKUI_GIT_BRANCH=\"${HAKUI_GIT_BRANCH}\"\n"
    "        HAKUI_BUILD_TIMESTAMP=\"${HAKUI_BUILD_TIMESTAMP}\"\n"
    "        HAKUI_BUILD_CONFIGURATION=\"${HAKUI_BUILD_CONFIGURATION}\"\n"
    "    )\n\n"
    "    if(WIN32)\n"
    "        target_link_libraries(hakui_female PRIVATE gdiplus)\n"
    "    endif()\n"
    "    if(MSVC)\n"
    "        target_compile_options(hakui_female PRIVATE /W4 /permissive-)\n"
    "    else()\n"
    "        target_compile_options(hakui_female PRIVATE -Wall -Wextra -Wpedantic)\n"
    "    endif()\n\n"
)
replace_exact(cmake, anchor, insert, "female HAKUI executable target")

# Regression guards: this copied game build must never masquerade as the lab.
app_text = app.read_text(encoding="utf-8")
if "scene.mannequinLab = true" in app_text:
    raise RuntimeError("HAKUI runtime unexpectedly activates mannequinLab")
if "scene.playerFemaleShell = kHakuiFemalePlayer" not in app_text:
    raise RuntimeError("female HAKUI scene activation missing")

print("[HAKUI FEMALE v1.01] copied polished HAKUI + female presentation shell complete")
