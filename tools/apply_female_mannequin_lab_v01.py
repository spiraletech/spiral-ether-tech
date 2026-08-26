from pathlib import Path


def replace_exact(path: Path, old: str, new: str, label: str) -> None:
    text = path.read_text(encoding="utf-8")
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{label}: expected exactly one match in {path}, found {count}")
    path.write_text(text.replace(old, new, 1), encoding="utf-8")
    print(f"[FEMALE MANNEQUIN LAB] patched {label}: {path}")


header = Path("src/render/DebugWorldRenderer.hpp")
renderer = Path("src/render/DebugWorldRenderer.cpp")
app = Path("src/mannequin_lab/MannequinLabApp.cpp")
cmake = Path("CMakeLists.txt")

# -----------------------------------------------------------------------------
# Separate presentation identity. The male v0.13 lab remains the baseline and
# the female executable opts into a sibling body shell through a compile define.
# -----------------------------------------------------------------------------
replace_exact(
    header,
    "    bool mannequinLab = false;\n"
    "    bool mannequinShowJoints = false;\n",
    "    bool mannequinLab = false;\n"
    "    bool mannequinFemale = false;\n"
    "    bool mannequinShowJoints = false;\n",
    "female scene flag",
)

replace_exact(
    app,
    "constexpr float kPi = 3.14159265358979323846f;\n\n"
    "} // namespace\n",
    "constexpr float kPi = 3.14159265358979323846f;\n\n"
    "#if defined(HAKUI_FEMALE_MANNEQUIN)\n"
    "constexpr bool kFemaleMannequinLab = true;\n"
    "constexpr const char* kLabWindowTitle =\n"
    "    \"HAKUI FEMALE MANNEQUIN LAB // RIG SCIENCE v0.1\";\n"
    "constexpr const char* kLabDynamicTitle =\n"
    "    \"HAKUI FEMALE MANNEQUIN LAB v0.1 // BASE RIG v0.13\";\n"
    "#else\n"
    "constexpr bool kFemaleMannequinLab = false;\n"
    "constexpr const char* kLabWindowTitle =\n"
    "    \"HAKUI MANNEQUIN LAB // RIG SCIENCE v0.13\";\n"
    "constexpr const char* kLabDynamicTitle =\n"
    "    \"HAKUI MANNEQUIN LAB v0.13\";\n"
    "#endif\n\n"
    "} // namespace\n",
    "female executable identity constants",
)

replace_exact(
    app,
    "    SDL_Log(\"[MANNEQUIN LAB] boot // silhouette pass v0.13\");\n",
    "    SDL_Log(\n"
    "        kFemaleMannequinLab\n"
    "            ? \"[FEMALE MANNEQUIN LAB] boot // base rig v0.13 // female shell v0.1\"\n"
    "            : \"[MANNEQUIN LAB] boot // silhouette pass v0.13\"\n"
    "    );\n",
    "female boot identity",
)

replace_exact(
    app,
    "    mannequin_.displayName = \"MANNEQUIN\";\n",
    "    mannequin_.displayName = kFemaleMannequinLab\n"
    "        ? \"FEMALE MANNEQUIN\"\n"
    "        : \"MANNEQUIN\";\n",
    "female mannequin display identity",
)

replace_exact(
    app,
    "        \"HAKUI MANNEQUIN LAB // RIG SCIENCE v0.13\",\n",
    "        kLabWindowTitle,\n",
    "female window title",
)

replace_exact(
    app,
    "        \"HAKUI MANNEQUIN LAB v0.13 // %.*s // PELVIS %.2f // TORSO %.2f // LEAN %.2f // KNEES %.2f/%.2f // JOINTS %s\",\n"
    "        static_cast<int>(poseLabel().size()), poseLabel().data(),\n",
    "        \"%s // %.*s // PELVIS %.2f // TORSO %.2f // LEAN %.2f // KNEES %.2f/%.2f // JOINTS %s\",\n"
    "        kLabDynamicTitle,\n"
    "        static_cast<int>(poseLabel().size()), poseLabel().data(),\n",
    "female dynamic title",
)

replace_exact(
    app,
    "    scene.mannequinLab = true;\n"
    "    scene.mannequinShowJoints = showJoints_;\n",
    "    scene.mannequinLab = true;\n"
    "    scene.mannequinFemale = kFemaleMannequinLab;\n"
    "    scene.mannequinShowJoints = showJoints_;\n",
    "female scene activation",
)

# -----------------------------------------------------------------------------
# Female shell geometry. Same skeleton, same pose math, same controls. Only the
# presentation shell changes: narrower shoulders, stronger waist taper, wider
# pelvis, fuller thighs, slimmer calves/wrists, smaller hands/feet/head.
# -----------------------------------------------------------------------------
replace_exact(
    renderer,
    "            scene.mannequinLab\n"
    "                ? Vec3{side * 0.52f, 2.04f, 0.0f}\n"
    "                : Vec3{side * 0.60f, 2.12f, 0.0f},\n",
    "            scene.mannequinLab\n"
    "                ? (scene.mannequinFemale\n"
    "                    ? Vec3{side * 0.46f, 2.02f, 0.0f}\n"
    "                    : Vec3{side * 0.52f, 2.04f, 0.0f})\n"
    "                : Vec3{side * 0.60f, 2.12f, 0.0f},\n",
    "female shoulder width and height",
)

replace_exact(
    renderer,
    "        const float elbowOut = scene.mannequinLab ? 0.075f : 0.12f;\n",
    "        const float elbowOut = scene.mannequinLab\n"
    "            ? (scene.mannequinFemale ? 0.060f : 0.075f)\n"
    "            : 0.12f;\n",
    "female elbow line",
)

replace_exact(
    renderer,
    "        contactSegment(shoulder, elbow, scene.mannequinLab ? 0.145f : 0.23f);\n"
    "        contactSegment(elbow, target, scene.mannequinLab ? 0.112f : 0.21f);\n",
    "        contactSegment(\n"
    "            shoulder, elbow,\n"
    "            scene.mannequinLab\n"
    "                ? (scene.mannequinFemale ? 0.132f : 0.145f)\n"
    "                : 0.23f\n"
    "        );\n"
    "        contactSegment(\n"
    "            elbow, target,\n"
    "            scene.mannequinLab\n"
    "                ? (scene.mannequinFemale ? 0.094f : 0.112f)\n"
    "                : 0.21f\n"
    "        );\n",
    "female arm taper",
)

replace_exact(
    renderer,
    "            scene.mannequinLab\n"
    "                ? Vec3{0.135f, 0.115f, 0.150f}\n"
    "                : Vec3{0.24f, 0.20f, 0.24f},\n",
    "            scene.mannequinLab\n"
    "                ? (scene.mannequinFemale\n"
    "                    ? Vec3{0.115f, 0.095f, 0.135f}\n"
    "                    : Vec3{0.135f, 0.115f, 0.150f})\n"
    "                : Vec3{0.24f, 0.20f, 0.24f},\n",
    "female hand scale",
)

replace_exact(
    renderer,
    "        contactSegment(hip, knee, scene.mannequinLab ? 0.188f : 0.27f);\n"
    "        contactSegment(knee, target, scene.mannequinLab ? 0.145f : 0.25f);\n",
    "        contactSegment(\n"
    "            hip, knee,\n"
    "            scene.mannequinLab\n"
    "                ? (scene.mannequinFemale ? 0.205f : 0.188f)\n"
    "                : 0.27f\n"
    "        );\n"
    "        contactSegment(\n"
    "            knee, target,\n"
    "            scene.mannequinLab\n"
    "                ? (scene.mannequinFemale ? 0.132f : 0.145f)\n"
    "                : 0.25f\n"
    "        );\n",
    "female thigh/calf taper",
)

replace_exact(
    renderer,
    "            scene.mannequinLab\n"
    "                ? Vec3{0.205f, 0.095f, 0.360f}\n"
    "                : Vec3{0.32f, 0.14f, 0.50f},\n",
    "            scene.mannequinLab\n"
    "                ? (scene.mannequinFemale\n"
    "                    ? Vec3{0.185f, 0.082f, 0.330f}\n"
    "                    : Vec3{0.205f, 0.095f, 0.360f})\n"
    "                : Vec3{0.32f, 0.14f, 0.50f},\n",
    "female foot scale",
)

replace_exact(
    renderer,
    "        Vec3 leftLabFoot = rotateYawPoint({-0.23f, 0.14f, 0.04f}, pelvisYaw);\n"
    "        Vec3 rightLabFoot = rotateYawPoint({0.23f, 0.14f, 0.04f}, pelvisYaw);\n",
    "        const float labFootSpread = scene.mannequinFemale ? 0.255f : 0.23f;\n"
    "        Vec3 leftLabFoot = rotateYawPoint(\n"
    "            {-labFootSpread, 0.14f, 0.04f}, pelvisYaw\n"
    "        );\n"
    "        Vec3 rightLabFoot = rotateYawPoint(\n"
    "            {labFootSpread, 0.14f, 0.04f}, pelvisYaw\n"
    "        );\n",
    "female stance width",
)

replace_exact(
    renderer,
    "            ? Vec3{0.54f, 0.20f, 0.32f}\n"
    "            : Vec3{0.74f, 0.30f, 0.44f},\n",
    "            ? (scene.mannequinFemale\n"
    "                ? Vec3{0.64f, 0.22f, 0.36f}\n"
    "                : Vec3{0.54f, 0.20f, 0.32f})\n"
    "            : Vec3{0.74f, 0.30f, 0.44f},\n",
    "female pelvis shell",
)

replace_exact(
    renderer,
    "            {0.46f, 0.13f, 0.30f},\n"
    "            scene.rideable.body.pelvisYawRelativeToBoard,\n",
    "            scene.mannequinFemale\n"
    "                ? Vec3{0.40f, 0.13f, 0.29f}\n"
    "                : Vec3{0.46f, 0.13f, 0.30f},\n"
    "            scene.rideable.body.pelvisYawRelativeToBoard,\n",
    "female waist bridge",
)

replace_exact(
    renderer,
    "                            scene.mannequinLab ? 0.58f : 0.92f,\n"
    "                            scene.mannequinLab ? 0.86f : 0.94f,\n"
    "                            scene.mannequinLab ? 0.31f : 0.48f\n",
    "                            scene.mannequinLab\n"
    "                                ? (scene.mannequinFemale ? 0.54f : 0.58f)\n"
    "                                : 0.92f,\n"
    "                            scene.mannequinLab\n"
    "                                ? (scene.mannequinFemale ? 0.84f : 0.86f)\n"
    "                                : 0.94f,\n"
    "                            scene.mannequinLab\n"
    "                                ? (scene.mannequinFemale ? 0.30f : 0.31f)\n"
    "                                : 0.48f\n",
    "female torso frame",
)

replace_exact(
    renderer,
    "                    scale({1.24f, 0.58f, 1.10f})\n",
    "                    scale({\n"
    "                        scene.mannequinFemale ? 1.16f : 1.24f,\n"
    "                        0.58f,\n"
    "                        scene.mannequinFemale ? 1.08f : 1.10f\n"
    "                    })\n",
    "female ribcage shape",
)

replace_exact(
    renderer,
    "                    scale({0.86f, 0.38f, 0.96f})\n",
    "                    scale({\n"
    "                        scene.mannequinFemale ? 0.72f : 0.86f,\n"
    "                        0.38f,\n"
    "                        scene.mannequinFemale ? 0.92f : 0.96f\n"
    "                    })\n",
    "female waist taper",
)

replace_exact(
    renderer,
    "        const Vec3 leftShoulder = rotateYawPoint({-0.52f, 2.04f, 0.0f}, torsoYaw);\n"
    "        const Vec3 rightShoulder = rotateYawPoint({0.52f, 2.04f, 0.0f}, torsoYaw);\n"
    "        contactSegment(clavicleCenter, leftShoulder, 0.095f, Shell);\n"
    "        contactSegment(clavicleCenter, rightShoulder, 0.095f, Shell);\n",
    "        const float clavicleSpread = scene.mannequinFemale ? 0.46f : 0.52f;\n"
    "        const float clavicleY = scene.mannequinFemale ? 2.02f : 2.04f;\n"
    "        const Vec3 leftShoulder = rotateYawPoint(\n"
    "            {-clavicleSpread, clavicleY, 0.0f}, torsoYaw\n"
    "        );\n"
    "        const Vec3 rightShoulder = rotateYawPoint(\n"
    "            {clavicleSpread, clavicleY, 0.0f}, torsoYaw\n"
    "        );\n"
    "        const float clavicleWidth = scene.mannequinFemale ? 0.082f : 0.095f;\n"
    "        contactSegment(clavicleCenter, leftShoulder, clavicleWidth, Shell);\n"
    "        contactSegment(clavicleCenter, rightShoulder, clavicleWidth, Shell);\n",
    "female clavicle line",
)

replace_exact(
    renderer,
    "            ? Vec3{0.145f, 0.205f, 0.145f}\n"
    "            : Vec3{0.22f, 0.18f, 0.22f},\n",
    "            ? (scene.mannequinFemale\n"
    "                ? Vec3{0.125f, 0.190f, 0.125f}\n"
    "                : Vec3{0.145f, 0.205f, 0.145f})\n"
    "            : Vec3{0.22f, 0.18f, 0.22f},\n",
    "female neck shell",
)

replace_exact(
    renderer,
    "            ? Vec3{0.42f, 0.50f, 0.40f}\n"
    "            : Vec3{0.56f, 0.58f, 0.52f},\n",
    "            ? (scene.mannequinFemale\n"
    "                ? Vec3{0.39f, 0.47f, 0.37f}\n"
    "                : Vec3{0.42f, 0.50f, 0.40f})\n"
    "            : Vec3{0.56f, 0.58f, 0.52f},\n",
    "female head shell",
)

# Female-specific arm reach targets while preserving the exact pose semantics.
replace_exact(
    renderer,
    "        Vec3 leftHand{-0.62f, 1.56f, 0.02f};\n"
    "        Vec3 rightHand{0.62f, 1.56f, 0.02f};\n",
    "        const float neutralHandX = scene.mannequinFemale ? 0.56f : 0.62f;\n"
    "        Vec3 leftHand{-neutralHandX, 1.56f, 0.02f};\n"
    "        Vec3 rightHand{neutralHandX, 1.56f, 0.02f};\n",
    "female neutral arm reach",
)

replace_exact(
    renderer,
    "            leftHand = {-1.28f, 2.04f, 0.0f};\n"
    "            rightHand = {1.28f, 2.04f, 0.0f};\n",
    "            leftHand = {scene.mannequinFemale ? -1.18f : -1.28f,\n"
    "                        scene.mannequinFemale ? 2.02f : 2.04f, 0.0f};\n"
    "            rightHand = {scene.mannequinFemale ? 1.18f : 1.28f,\n"
    "                         scene.mannequinFemale ? 2.02f : 2.04f, 0.0f};\n",
    "female T-pose reach",
)

replace_exact(
    renderer,
    "            leftHand = {-1.05f, 1.72f, 0.0f};\n"
    "            rightHand = {1.05f, 1.72f, 0.0f};\n",
    "            leftHand = {scene.mannequinFemale ? -0.96f : -1.05f,\n"
    "                        scene.mannequinFemale ? 1.70f : 1.72f, 0.0f};\n"
    "            rightHand = {scene.mannequinFemale ? 0.96f : 1.05f,\n"
    "                         scene.mannequinFemale ? 1.70f : 1.72f, 0.0f};\n",
    "female A-pose reach",
)

replace_exact(
    renderer,
    "            leftHand = {-0.72f, 1.40f, -0.08f};\n"
    "            rightHand = {0.72f, 1.40f, -0.08f};\n",
    "            leftHand = {scene.mannequinFemale ? -0.66f : -0.72f,\n"
    "                        1.40f, -0.08f};\n"
    "            rightHand = {scene.mannequinFemale ? 0.66f : 0.72f,\n"
    "                         1.40f, -0.08f};\n",
    "female crouch arm reach",
)

replace_exact(
    renderer,
    "            leftHand = {-0.92f, 1.58f, -0.14f};\n"
    "            rightHand = {0.92f, 1.68f, 0.18f};\n",
    "            leftHand = {scene.mannequinFemale ? -0.84f : -0.92f,\n"
    "                        1.58f, -0.14f};\n"
    "            rightHand = {scene.mannequinFemale ? 0.84f : 0.92f,\n"
    "                         1.68f, 0.18f};\n",
    "female ollie-pop arm reach",
)

replace_exact(
    renderer,
    "            \"HAKUI MANNEQUIN LAB // RIG SCIENCE\",\n",
    "            scene.mannequinFemale\n"
    "                ? \"HAKUI FEMALE MANNEQUIN LAB // RIG SCIENCE\"\n"
    "                : \"HAKUI MANNEQUIN LAB // RIG SCIENCE\",\n",
    "female HUD identity",
)

# -----------------------------------------------------------------------------
# Build a true sibling executable using the same app/renderer source with a
# compile definition selecting the female shell. No runtime toggle is needed.
# -----------------------------------------------------------------------------
replace_exact(
    cmake,
    "    if(WIN32)\n"
    "        target_link_libraries(hakui PRIVATE gdiplus)\n",
    "    # ------------------------------------------------------------\n"
    "    # HAKUI FEMALE MANNEQUIN LAB\n"
    "    # ------------------------------------------------------------\n"
    "    add_executable(hakui_female_mannequin_lab\n"
    "        src/mannequin_lab/main.cpp\n"
    "        src/mannequin_lab/MannequinLabApp.cpp\n"
    "        src/mannequin_lab/MannequinLabApp.hpp\n"
    "        src/render/DebugWorldRenderer.cpp\n"
    "        src/render/DebugWorldRenderer.hpp\n"
    "        src/render/Math3D.hpp\n"
    "        src/player/PlayerState.hpp\n"
    "        src/world/WorldGeometry.hpp\n"
    "    )\n\n"
    "    set_target_properties(hakui_female_mannequin_lab PROPERTIES\n"
    "        OUTPUT_NAME \"HAKUI-FEMALE-MANNEQUIN-LAB\"\n"
    "    )\n\n"
    "    target_compile_definitions(hakui_female_mannequin_lab PRIVATE\n"
    "        HAKUI_FEMALE_MANNEQUIN=1\n"
    "    )\n\n"
    "    target_link_libraries(hakui_female_mannequin_lab\n"
    "        PRIVATE\n"
    "            SDL3::SDL3\n"
    "            hakui_avatar_rig\n"
    "            hakui_gameplay\n"
    "            hakui_social\n"
    "            hakui_camera\n"
    "            hakui_combat\n"
    "    )\n\n"
    "    target_include_directories(hakui_female_mannequin_lab PRIVATE\n"
    "        src\n"
    "        \"${sdl3_SOURCE_DIR}/test\"\n"
    "    )\n\n"
    "    if(WIN32)\n"
    "        target_link_libraries(hakui_female_mannequin_lab PRIVATE gdiplus)\n"
    "    endif()\n"
    "    if(MSVC)\n"
    "        target_compile_options(hakui_female_mannequin_lab PRIVATE /W4 /permissive-)\n"
    "    else()\n"
    "        target_compile_options(hakui_female_mannequin_lab PRIVATE -Wall -Wextra -Wpedantic)\n"
    "    endif()\n\n"
    "    if(WIN32)\n"
    "        target_link_libraries(hakui PRIVATE gdiplus)\n",
    "female executable target",
)

print("[FEMALE MANNEQUIN LAB] separate female program + body shell complete")
