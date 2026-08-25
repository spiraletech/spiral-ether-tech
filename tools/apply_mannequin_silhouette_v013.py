from pathlib import Path


def replace_exact(path: Path, old: str, new: str, label: str) -> None:
    text = path.read_text(encoding="utf-8")
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{label}: expected exactly one match in {path}, found {count}")
    path.write_text(text.replace(old, new, 1), encoding="utf-8")
    print(f"[MANNEQUIN v0.13] patched {label}: {path}")


def replace_all(path: Path, old: str, new: str, label: str) -> None:
    text = path.read_text(encoding="utf-8")
    count = text.count(old)
    if count == 0:
        raise RuntimeError(f"{label}: no matches in {path}")
    path.write_text(text.replace(old, new), encoding="utf-8")
    print(f"[MANNEQUIN v0.13] patched {label} ({count} matches): {path}")


renderer = Path("src/render/DebugWorldRenderer.cpp")
app = Path("src/mannequin_lab/MannequinLabApp.cpp")

# ------------------------------------------------------------
# Shoulder / clavicle science
# ------------------------------------------------------------
# Lower and narrow the shoulder pivot only for the lab. The shoulder is now
# attached beneath a slight clavicle slope instead of reading as a horizontal
# robot-bar stabbed into the torso.
replace_exact(
    renderer,
    "        const Vec3 shoulder = rotateYawPoint(\n"
    "            {side * 0.60f, 2.12f, 0.0f},\n"
    "            shoulderYaw\n"
    "        );\n",
    "        const Vec3 shoulder = rotateYawPoint(\n"
    "            scene.mannequinLab\n"
    "                ? Vec3{side * 0.52f, 2.04f, 0.0f}\n"
    "                : Vec3{side * 0.60f, 2.12f, 0.0f},\n"
    "            shoulderYaw\n"
    "        );\n",
    "lower/narrow lab shoulder pivots",
)

# Subtle elbow bias and progressively slimmer upper/lower arm segments.
replace_exact(
    renderer,
    "        const Vec3 elbow{\n"
    "            (shoulder.x + target.x) * 0.5f + side * 0.12f,\n"
    "            (shoulder.y + target.y) * 0.5f - 0.04f - elbowFlex * 0.06f,\n"
    "            (shoulder.z + target.z) * 0.5f - 0.08f - elbowFlex * 0.08f\n"
    "        };\n",
    "        const float elbowOut = scene.mannequinLab ? 0.075f : 0.12f;\n"
    "        const Vec3 elbow{\n"
    "            (shoulder.x + target.x) * 0.5f + side * elbowOut,\n"
    "            (shoulder.y + target.y) * 0.5f - 0.04f - elbowFlex * 0.06f,\n"
    "            (shoulder.z + target.z) * 0.5f - 0.08f - elbowFlex * 0.08f\n"
    "        };\n",
    "lab elbow anatomical bias",
)
replace_exact(
    renderer,
    "        contactSegment(shoulder, elbow, scene.mannequinLab ? 0.155f : 0.23f);\n"
    "        contactSegment(elbow, target, scene.mannequinLab ? 0.135f : 0.21f);\n",
    "        contactSegment(shoulder, elbow, scene.mannequinLab ? 0.145f : 0.23f);\n"
    "        contactSegment(elbow, target, scene.mannequinLab ? 0.112f : 0.21f);\n",
    "tapered lab arms",
)
replace_exact(
    renderer,
    "            scene.mannequinLab\n"
    "                ? Vec3{0.16f, 0.15f, 0.18f}\n"
    "                : Vec3{0.24f, 0.20f, 0.24f},\n",
    "            scene.mannequinLab\n"
    "                ? Vec3{0.135f, 0.115f, 0.150f}\n"
    "                : Vec3{0.24f, 0.20f, 0.24f},\n",
    "smaller mannequin hands",
)

# ------------------------------------------------------------
# Thigh -> calf -> foot taper
# ------------------------------------------------------------
replace_exact(
    renderer,
    "        contactSegment(hip, knee, scene.mannequinLab ? 0.205f : 0.27f);\n"
    "        contactSegment(knee, target, scene.mannequinLab ? 0.175f : 0.25f);\n",
    "        contactSegment(hip, knee, scene.mannequinLab ? 0.188f : 0.27f);\n"
    "        contactSegment(knee, target, scene.mannequinLab ? 0.145f : 0.25f);\n",
    "thigh-to-calf taper",
)
replace_exact(
    renderer,
    "            scene.mannequinLab\n"
    "                ? Vec3{0.24f, 0.11f, 0.42f}\n"
    "                : Vec3{0.32f, 0.14f, 0.50f},\n",
    "            scene.mannequinLab\n"
    "                ? Vec3{0.205f, 0.095f, 0.360f}\n"
    "                : Vec3{0.32f, 0.14f, 0.50f},\n",
    "smaller mannequin feet",
)

# ------------------------------------------------------------
# Pelvis wedge / waist bridge
# ------------------------------------------------------------
replace_exact(
    renderer,
    "            ? Vec3{0.60f, 0.24f, 0.36f}\n"
    "            : Vec3{0.74f, 0.30f, 0.44f},\n",
    "            ? Vec3{0.54f, 0.20f, 0.32f}\n"
    "            : Vec3{0.74f, 0.30f, 0.44f},\n",
    "cleaner pelvis mass",
)
replace_exact(
    renderer,
    "        Midnight\n"
    "    );\n\n"
    "    const bool attackRelease = scene.combatActive &&\n",
    "        Midnight\n"
    "    );\n"
    "    if (scene.mannequinLab) {\n"
    "        // Upper pelvis/waist bridge: stacked geometry gives the lab body\n"
    "        // a simple wedge transition instead of torso -> belt -> legs.\n"
    "        orientedLocalBox(\n"
    "            {0.0f,\n"
    "             1.32f - scene.rideable.body.preloadPoseWeight * 0.16f -\n"
    "                 scene.rideable.body.landingCompression * 0.13f,\n"
    "             0.0f},\n"
    "            {0.46f, 0.13f, 0.30f},\n"
    "            scene.rideable.body.pelvisYawRelativeToBoard,\n"
    "            Midnight\n"
    "        );\n"
    "    }\n\n"
    "    const bool attackRelease = scene.combatActive &&\n",
    "pelvis-to-waist bridge",
)

# ------------------------------------------------------------
# Ribcage -> waist taper
# ------------------------------------------------------------
# The base torso becomes the transform frame. In lab mode we render two child
# masses instead of the single refrigerator box: a broader ribcage and narrower
# waist, both inheriting pose yaw/lean/roll from the existing torso matrix.
replace_exact(
    renderer,
    "                            scene.mannequinLab ? 0.74f : 0.92f,\n"
    "                            scene.mannequinLab ? 0.90f : 0.94f,\n"
    "                            scene.mannequinLab ? 0.38f : 0.48f\n",
    "                            scene.mannequinLab ? 0.58f : 0.92f,\n"
    "                            scene.mannequinLab ? 0.86f : 0.94f,\n"
    "                            scene.mannequinLab ? 0.31f : 0.48f\n",
    "lab torso transform frame",
)
replace_exact(
    renderer,
    "    drawModel(torso, scene.playerHitPulse > 0.0f ? Danger : Shell);\n",
    "    const Uint32 torsoPalette = scene.playerHitPulse > 0.0f ? Danger : Shell;\n"
    "    if (scene.mannequinLab) {\n"
    "        drawModel(\n"
    "            multiply(\n"
    "                torso,\n"
    "                multiply(\n"
    "                    translation({0.0f, 0.18f, 0.0f}),\n"
    "                    scale({1.24f, 0.58f, 1.10f})\n"
    "                )\n"
    "            ),\n"
    "            torsoPalette\n"
    "        );\n"
    "        drawModel(\n"
    "            multiply(\n"
    "                torso,\n"
    "                multiply(\n"
    "                    translation({0.0f, -0.28f, 0.0f}),\n"
    "                    scale({0.86f, 0.38f, 0.96f})\n"
    "                )\n"
    "            ),\n"
    "            torsoPalette\n"
    "        );\n"
    "    } else {\n"
    "        drawModel(torso, torsoPalette);\n"
    "    }\n",
    "ribcage-to-waist taper",
)

# Add actual clavicle geometry after the torso has been established. This is a
# small visual bridge, not a new skeletal authority.
replace_exact(
    renderer,
    "    float leftArmAngle = (seated ? -0.62f : 0.0f) + counterGait * armStride;\n",
    "    if (scene.mannequinLab) {\n"
    "        const float torsoYaw = scene.rideable.body.torsoYawRelativeToBoard;\n"
    "        const Vec3 clavicleCenter = rotateYawPoint({0.0f, 2.12f, 0.0f}, torsoYaw);\n"
    "        const Vec3 leftShoulder = rotateYawPoint({-0.52f, 2.04f, 0.0f}, torsoYaw);\n"
    "        const Vec3 rightShoulder = rotateYawPoint({0.52f, 2.04f, 0.0f}, torsoYaw);\n"
    "        contactSegment(clavicleCenter, leftShoulder, 0.095f, Shell);\n"
    "        contactSegment(clavicleCenter, rightShoulder, 0.095f, Shell);\n"
    "    }\n\n"
    "    float leftArmAngle = (seated ? -0.62f : 0.0f) + counterGait * armStride;\n",
    "sloped mannequin clavicles",
)

# ------------------------------------------------------------
# Head / neck connection
# ------------------------------------------------------------
replace_exact(
    renderer,
    "            ? Vec3{0.16f, 0.17f, 0.16f}\n"
    "            : Vec3{0.22f, 0.18f, 0.22f},\n",
    "            ? Vec3{0.145f, 0.205f, 0.145f}\n"
    "            : Vec3{0.22f, 0.18f, 0.22f},\n",
    "cleaner mannequin neck",
)
replace_exact(
    renderer,
    "            ? Vec3{0.46f, 0.54f, 0.44f}\n"
    "            : Vec3{0.56f, 0.58f, 0.52f},\n",
    "            ? Vec3{0.42f, 0.50f, 0.40f}\n"
    "            : Vec3{0.56f, 0.58f, 0.52f},\n",
    "smaller mannequin head",
)

# Version the dedicated lab only. HAKUI runtime/version strings are untouched.
replace_all(app, "v0.1", "v0.13", "lab version labels")
replace_all(app, "rig science v0.13", "silhouette pass v0.13", "lab boot milestone")

print("[MANNEQUIN v0.13] SILHOUETTE PASS complete")
