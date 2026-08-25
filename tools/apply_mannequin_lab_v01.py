from pathlib import Path
import textwrap


def replace_exact(path: Path, old: str, new: str, label: str) -> None:
    text = path.read_text(encoding="utf-8")
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{label}: expected exactly one match in {path}, found {count}")
    path.write_text(text.replace(old, new, 1), encoding="utf-8")
    print(f"[MANNEQUIN LAB] patched {label}: {path}")


header = Path("src/render/DebugWorldRenderer.hpp")
renderer = Path("src/render/DebugWorldRenderer.cpp")

# A tiny lab-only presentation seam. Gameplay HAKUI never sets these fields.
replace_exact(
    header,
    "struct HakuiSceneState {\n    bool paused = false;\n",
    "struct HakuiSceneState {\n"
    "    bool mannequinLab = false;\n"
    "    bool mannequinShowJoints = false;\n"
    "    std::uint8_t mannequinPosePreset = 0;\n"
    "    std::string_view mannequinPoseLabel{\"NEUTRAL\"};\n"
    "    bool paused = false;\n",
    "lab scene presentation fields",
)

text = renderer.read_text(encoding="utf-8")

# Strip Black Room terminal/card decoration from the lab render path while
# leaving the normal Hakui path byte-for-byte inside the branch.
start_marker = "    // Runtime state decorates reusable geometry without owning its layout.\n"
end_marker = "    // Locomotion embodiment is presentation driven by deterministic player\n"
a = text.find(start_marker)
b = text.find(end_marker, a)
if a < 0 or b < 0:
    raise RuntimeError("terminal decoration block markers not found")
block = text[a:b]
wrapped = "    if (!scene.mannequinLab) {\n" + textwrap.indent(block, "    ") + "    }\n\n"
text = text[:a] + wrapped + text[b:]
renderer.write_text(text, encoding="utf-8")
print("[MANNEQUIN LAB] isolated world decoration")

# The lab is pose-driven even though it is not mounted on a rideable.
replace_exact(
    renderer,
    "    const bool mounted = (ridingSkateboard || ridingBmx) && !rideBail;\n"
    "    const bool socialAllowed = !mounted && !rideBail && !scene.combatActive;\n",
    "    const bool mounted = (ridingSkateboard || ridingBmx) && !rideBail;\n"
    "    const bool poseDriven = mounted || scene.mannequinLab;\n"
    "    const bool socialAllowed = !poseDriven && !rideBail && !scene.combatActive;\n",
    "lab pose-driven embodiment gate",
)

# Make the contact solver read like a clean mannequin rather than a block toy
# when the dedicated lab flag is active. Joint positions remain the same math.
replace_exact(
    renderer,
    "        contactSegment(hip, knee, 0.27f);\n"
    "        contactSegment(knee, target, 0.25f);\n"
    "        const Vec3 footForward = rotateYawPoint({0.0f, 0.0f, 0.08f}, footYaw);\n"
    "        orientedLocalBox(\n"
    "            {target.x + footForward.x,\n"
    "             target.y + 0.06f,\n"
    "             target.z + footForward.z},\n"
    "            {0.32f, 0.14f, 0.50f},\n",
    "        contactSegment(hip, knee, scene.mannequinLab ? 0.205f : 0.27f);\n"
    "        contactSegment(knee, target, scene.mannequinLab ? 0.175f : 0.25f);\n"
    "        if (scene.mannequinLab && scene.mannequinShowJoints) {\n"
    "            localBox(hip, {0.12f, 0.12f, 0.12f}, Amber);\n"
    "            localBox(knee, {0.13f, 0.13f, 0.13f}, Magenta);\n"
    "            localBox(target, {0.11f, 0.11f, 0.11f}, Cyan);\n"
    "        }\n"
    "        const Vec3 footForward = rotateYawPoint({0.0f, 0.0f, 0.08f}, footYaw);\n"
    "        orientedLocalBox(\n"
    "            {target.x + footForward.x,\n"
    "             target.y + 0.06f,\n"
    "             target.z + footForward.z},\n"
    "            scene.mannequinLab\n"
    "                ? Vec3{0.24f, 0.11f, 0.42f}\n"
    "                : Vec3{0.32f, 0.14f, 0.50f},\n",
    "humanized lab leg widths and joint markers",
)

replace_exact(
    renderer,
    "        contactSegment(shoulder, elbow, 0.23f);\n"
    "        contactSegment(elbow, target, 0.21f);\n"
    "        localBox(target, {0.24f, 0.20f, 0.24f}, Midnight);\n",
    "        contactSegment(shoulder, elbow, scene.mannequinLab ? 0.155f : 0.23f);\n"
    "        contactSegment(elbow, target, scene.mannequinLab ? 0.135f : 0.21f);\n"
    "        if (scene.mannequinLab && scene.mannequinShowJoints) {\n"
    "            localBox(shoulder, {0.12f, 0.12f, 0.12f}, Amber);\n"
    "            localBox(elbow, {0.12f, 0.12f, 0.12f}, Magenta);\n"
    "            localBox(target, {0.10f, 0.10f, 0.10f}, Cyan);\n"
    "        }\n"
    "        localBox(\n"
    "            target,\n"
    "            scene.mannequinLab\n"
    "                ? Vec3{0.16f, 0.15f, 0.18f}\n"
    "                : Vec3{0.24f, 0.20f, 0.24f},\n"
    "            Midnight\n"
    "        );\n",
    "humanized lab arm widths and joint markers",
)

# Dedicated lab feet: no skateboard mesh, no gameplay mount. Preset 5 reuses
# the proven v1.01 front/rear pop semantics as a pure pose-study sample.
replace_exact(
    renderer,
    "    if (seated) {\n"
    "        contactLeg(-1.0f, {-0.34f, 0.34f, 0.58f}, 0.98f, 0.0f, 0.0f);\n",
    "    if (scene.mannequinLab) {\n"
    "        const float pelvisYaw = scene.rideable.body.pelvisYawRelativeToBoard;\n"
    "        Vec3 leftLabFoot = rotateYawPoint({-0.23f, 0.14f, 0.04f}, pelvisYaw);\n"
    "        Vec3 rightLabFoot = rotateYawPoint({0.23f, 0.14f, 0.04f}, pelvisYaw);\n"
    "        if (scene.mannequinPosePreset == 4) {\n"
    "            leftLabFoot.y += scene.rideable.body.frontFootLift;\n"
    "            rightLabFoot.y += scene.rideable.body.rearLegDrive * 0.10f;\n"
    "            rightLabFoot.z -= scene.rideable.body.rearLegDrive * 0.12f;\n"
    "        }\n"
    "        contactLeg(\n"
    "            -1.0f, leftLabFoot, scene.rideable.body.leftKneeFlex,\n"
    "            pelvisYaw, pelvisYaw\n"
    "        );\n"
    "        contactLeg(\n"
    "            1.0f, rightLabFoot, scene.rideable.body.rightKneeFlex,\n"
    "            pelvisYaw, pelvisYaw\n"
    "        );\n"
    "    } else if (seated) {\n"
    "        contactLeg(-1.0f, {-0.34f, 0.34f, 0.58f}, 0.98f, 0.0f, 0.0f);\n",
    "lab foot targets",
)

# Pelvis/torso/head consume pose data in the lab without pretending the avatar
# is riding anything.
text = renderer.read_text(encoding="utf-8")
text = text.replace(
    "        mounted ? scene.rideable.body.pelvisYawRelativeToBoard : 0.0f,\n",
    "        poseDriven ? scene.rideable.body.pelvisYawRelativeToBoard : 0.0f,\n",
    1,
)
text = text.replace(
    "    const float rideCompression = mounted\n",
    "    const float rideCompression = poseDriven\n",
    1,
)
text = text.replace(
    "                    mounted ? scene.rideable.body.torsoYawRelativeToBoard : 0.0f\n",
    "                    poseDriven ? scene.rideable.body.torsoYawRelativeToBoard : 0.0f\n",
    1,
)
text = text.replace(
    "                        (mounted ? scene.rideable.body.torsoLean : 0.0f) +\n",
    "                        (poseDriven ? scene.rideable.body.torsoLean : 0.0f) +\n",
    1,
)
text = text.replace(
    "    const float headYaw = mounted\n",
    "    const float headYaw = poseDriven\n",
    1,
)
renderer.write_text(text, encoding="utf-8")
print("[MANNEQUIN LAB] connected pelvis/torso/head pose seam")

# Human-ish mannequin proportions are presentation-only and exist only in this
# EXE path. HAKUI keeps its block truth rig.
replace_exact(
    renderer,
    "        {0.74f, 0.30f, 0.44f},\n"
    "        poseDriven ? scene.rideable.body.pelvisYawRelativeToBoard : 0.0f,\n",
    "        scene.mannequinLab\n"
    "            ? Vec3{0.60f, 0.24f, 0.36f}\n"
    "            : Vec3{0.74f, 0.30f, 0.44f},\n"
    "        poseDriven ? scene.rideable.body.pelvisYawRelativeToBoard : 0.0f,\n",
    "lab pelvis proportions",
)

replace_exact(
    renderer,
    "                        scale({0.92f, 0.94f, 0.48f})\n",
    "                        scale({\n"
    "                            scene.mannequinLab ? 0.74f : 0.92f,\n"
    "                            scene.mannequinLab ? 0.90f : 0.94f,\n"
    "                            scene.mannequinLab ? 0.38f : 0.48f\n"
    "                        })\n",
    "lab torso proportions",
)

# Lab-specific arm target presets.
replace_exact(
    renderer,
    "    if (mounted && ridingBmx) {\n"
    "        contactArm(\n",
    "    if (scene.mannequinLab) {\n"
    "        Vec3 leftHand{-0.62f, 1.56f, 0.02f};\n"
    "        Vec3 rightHand{0.62f, 1.56f, 0.02f};\n"
    "        switch (scene.mannequinPosePreset) {\n"
    "        case 1: // T-pose\n"
    "            leftHand = {-1.28f, 2.04f, 0.0f};\n"
    "            rightHand = {1.28f, 2.04f, 0.0f};\n"
    "            break;\n"
    "        case 2: // A-pose\n"
    "            leftHand = {-1.05f, 1.72f, 0.0f};\n"
    "            rightHand = {1.05f, 1.72f, 0.0f};\n"
    "            break;\n"
    "        case 3: // crouch\n"
    "            leftHand = {-0.72f, 1.40f, -0.08f};\n"
    "            rightHand = {0.72f, 1.40f, -0.08f};\n"
    "            break;\n"
    "        case 4: // ollie pop\n"
    "            leftHand = {-0.92f, 1.58f, -0.14f};\n"
    "            rightHand = {0.92f, 1.68f, 0.18f};\n"
    "            break;\n"
    "        default:\n"
    "            break;\n"
    "        }\n"
    "        leftHand = rotateYawPoint(\n"
    "            leftHand, scene.rideable.body.torsoYawRelativeToBoard\n"
    "        );\n"
    "        rightHand = rotateYawPoint(\n"
    "            rightHand, scene.rideable.body.torsoYawRelativeToBoard\n"
    "        );\n"
    "        contactArm(\n"
    "            -1.0f, leftHand, scene.rideable.body.torsoYawRelativeToBoard,\n"
    "            scene.rideable.body.leftElbowFlex\n"
    "        );\n"
    "        contactArm(\n"
    "            1.0f, rightHand, scene.rideable.body.torsoYawRelativeToBoard,\n"
    "            scene.rideable.body.rightElbowFlex\n"
    "        );\n"
    "    } else if (mounted && ridingBmx) {\n"
    "        contactArm(\n",
    "lab arm target presets",
)

# Slightly less cube-headed while in mannequin science mode.
replace_exact(
    renderer,
    "        {0.22f, 0.18f, 0.22f},\n        Cyan\n    );\n",
    "        scene.mannequinLab\n"
    "            ? Vec3{0.16f, 0.17f, 0.16f}\n"
    "            : Vec3{0.22f, 0.18f, 0.22f},\n"
    "        scene.mannequinLab ? Shell : Cyan\n    );\n",
    "lab neck proportions",
)
replace_exact(
    renderer,
    "        {0.56f, 0.58f, 0.52f},\n        Shell\n    );\n",
    "        scene.mannequinLab\n"
    "            ? Vec3{0.46f, 0.54f, 0.44f}\n"
    "            : Vec3{0.56f, 0.58f, 0.52f},\n"
    "        Shell\n    );\n",
    "lab head proportions",
)

# Minimal on-screen lab readout. No HAKUI chat/combat/casino HUD.
replace_exact(
    renderer,
    "    if (!scene.localDisplayName.empty()) {\n",
    "    if (scene.mannequinLab) {\n"
    "        bindGlass(0.94f);\n"
    "        drawClipText(\n"
    "            \"HAKUI MANNEQUIN LAB // RIG SCIENCE\",\n"
    "            -0.94f, 0.91f, 0.0040f, 0.0072f, Cyan\n"
    "        );\n"
    "        std::string poseLine = \"POSE // \";\n"
    "        poseLine += std::string{scene.mannequinPoseLabel};\n"
    "        poseLine += scene.mannequinShowJoints ? \" // JOINTS ON\" : \" // JOINTS OFF\";\n"
    "        drawClipText(poseLine, -0.94f, 0.82f, 0.0037f, 0.0067f, Amber);\n"
    "        drawClipText(\n"
    "            \"1-5 POSES // Q/E PELVIS // A/D TORSO // W/S LEAN // [ ] KNEES // J JOINTS\",\n"
    "            -0.94f, -0.83f, 0.0027f, 0.0052f, Shell\n"
    "        );\n"
    "        bindOpaque();\n"
    "    }\n\n"
    "    if (!scene.localDisplayName.empty()) {\n",
    "lab overlay",
)

print("[MANNEQUIN LAB] renderer isolation + rig visualization complete")
