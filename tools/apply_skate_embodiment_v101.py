from pathlib import Path


def replace_exact(path: Path, old: str, new: str, label: str) -> None:
    text = path.read_text(encoding="utf-8")
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{label}: expected exactly one match in {path}, found {count}")
    path.write_text(text.replace(old, new, 1), encoding="utf-8")
    print(f"[HAKUI v1.01] patched {label}: {path}")


def replace_between(path: Path, start: str, end: str, replacement: str, label: str) -> None:
    text = path.read_text(encoding="utf-8")
    a = text.find(start)
    if a < 0:
        raise RuntimeError(f"{label}: start marker not found in {path}")
    b = text.find(end, a + len(start))
    if b < 0:
        raise RuntimeError(f"{label}: end marker not found in {path}")
    path.write_text(text[:a] + replacement + text[b:], encoding="utf-8")
    print(f"[HAKUI v1.01] patched {label}: {path}")


def replace_all(path: Path, old: str, new: str, label: str) -> None:
    text = path.read_text(encoding="utf-8")
    count = text.count(old)
    if count == 0:
        raise RuntimeError(f"{label}: no matches in {path}")
    path.write_text(text.replace(old, new), encoding="utf-8")
    print(f"[HAKUI v1.01] patched {label} ({count} matches): {path}")


ride = Path("src/player/RideableMovementController.cpp")
renderer = Path("src/render/DebugWorldRenderer.cpp")
app = Path("src/core/HakuiApp.cpp")
tests = Path("tests/hakui/RideableMovementSpec.cpp")

# Replace the skateboard body-mechanics block with stance-mirrored front/rear
# semantics and a clearer preload -> pop -> rise -> level -> descent sequence.
skate_start = "    if (state_.discipline == RideDiscipline::Skateboard) {\n"
skate_end = "    } else if (state_.discipline == RideDiscipline::BMX) {\n"
skate_block = '''    if (state_.discipline == RideDiscipline::Skateboard) {\n        const float stanceDirection = skateStance_ == SkateStance::Regular\n            ? 1.0f\n            : -1.0f;\n        const auto setFrontRearKnees = [&](float front, float rear) {\n            if (skateStance_ == SkateStance::Regular) {\n                body.leftKneeFlex = front;\n                body.rightKneeFlex = rear;\n            } else {\n                body.leftKneeFlex = rear;\n                body.rightKneeFlex = front;\n            }\n        };\n\n        // Sideways board stance: hips commit almost fully across the deck,\n        // chest opens toward travel, head stays readable to the camera.\n        body.pelvisYawRelativeToBoard = stanceDirection * 1.50f;\n        body.torsoYawRelativeToBoard = stanceDirection * 0.58f;\n        body.headYawRelativeToBoard = stanceDirection * 0.10f;\n        setFrontRearKnees(\n            0.30f + preload * 0.82f,\n            0.36f + preload * 0.98f\n        );\n        body.leftElbowFlex = 0.26f + preload * 0.08f;\n        body.rightElbowFlex = 0.28f + preload * 0.08f;\n        body.torsoLean = 0.03f + preload * 0.20f;\n        body.armCounterbalance = state_.steeringVisual * 0.18f -\n            stanceDirection * preload * 0.10f;\n\n        if (state_.phase == RidePhase::Manual) {\n            body.leftKneeFlex += 0.16f;\n            body.rightKneeFlex += 0.28f;\n            body.torsoLean = -0.12f;\n            body.armCounterbalance = state_.balanceOffset * 0.82f;\n        } else if (state_.phase == RidePhase::Grinding) {\n            body.leftKneeFlex += 0.20f;\n            body.rightKneeFlex += 0.20f;\n            body.torsoLean = 0.09f;\n            body.armCounterbalance = state_.balanceOffset * 0.95f;\n        } else if (state_.phase == RidePhase::Airborne) {\n            const float airProgress = std::clamp(\n                state_.airSeconds / std::max(0.55f, state_.minimumTrickAirtime),\n                0.0f,\n                1.0f\n            );\n            if (state_.flipCommitted) {\n                body.footContact = state_.rotationCompletion < 0.66f\n                    ? RideFootContactState::ReleasedForTrick\n                    : RideFootContactState::Reacquiring;\n                switch (state_.activeTrick) {\n                    case RideTrick::Kickflip:\n                        body.airPose = RideAirPose::Kickflip;\n                        setFrontRearKnees(1.08f, 0.72f);\n                        body.frontFootLift = 0.26f;\n                        body.armCounterbalance = -stanceDirection * 0.68f;\n                        break;\n                    case RideTrick::Heelflip:\n                        body.airPose = RideAirPose::Heelflip;\n                        setFrontRearKnees(0.84f, 1.02f);\n                        body.frontFootLift = 0.18f;\n                        body.armCounterbalance = stanceDirection * 0.70f;\n                        break;\n                    case RideTrick::PopShoveIt:\n                        body.airPose = RideAirPose::PopShoveIt;\n                        setFrontRearKnees(0.82f, 0.72f);\n                        body.rearLegDrive = 0.32f;\n                        body.torsoYawRelativeToBoard += stanceDirection * 0.22f;\n                        break;\n                    case RideTrick::Impossible:\n                        body.airPose = RideAirPose::Impossible;\n                        setFrontRearKnees(1.14f, 0.60f);\n                        body.frontFootLift = 0.34f;\n                        body.rearLegDrive = 0.40f;\n                        body.armCounterbalance = -stanceDirection * 0.42f;\n                        break;\n                    case RideTrick::VarialFlip:\n                        body.airPose = RideAirPose::VarialFlip;\n                        setFrontRearKnees(1.02f, 0.88f);\n                        body.frontFootLift = 0.28f;\n                        body.rearLegDrive = 0.28f;\n                        body.armCounterbalance = stanceDirection * 0.50f;\n                        break;\n                    case RideTrick::BoardGrab:\n                        body.airPose = RideAirPose::BoardGrab;\n                        setFrontRearKnees(1.18f, 1.08f);\n                        body.torsoLean = 0.32f;\n                        break;\n                    default:\n                        body.airPose = RideAirPose::OllieLevel;\n                        setFrontRearKnees(0.90f, 0.86f);\n                        break;\n                }\n            } else {\n                body.footContact = airProgress < 0.22f\n                    ? RideFootContactState::ReleasedForTrick\n                    : RideFootContactState::Reacquiring;\n                if (state_.airSeconds < 0.10f) {\n                    // Rear leg snaps the tail; front leg is already rising.\n                    body.airPose = RideAirPose::OlliePop;\n                    body.rearLegDrive = 0.56f;\n                    body.frontFootLift = 0.08f;\n                    setFrontRearKnees(0.72f, 0.32f);\n                    body.armCounterbalance = -stanceDirection * 0.28f;\n                } else if (player.velocityY > 1.0f) {\n                    body.airPose = RideAirPose::OllieRise;\n                    body.frontFootLift = 0.34f;\n                    body.rearLegDrive = 0.18f;\n                    setFrontRearKnees(1.08f, 0.72f);\n                    body.armCounterbalance = stanceDirection * 0.18f;\n                } else if (player.velocityY > -1.2f) {\n                    body.airPose = RideAirPose::OllieLevel;\n                    body.frontFootLift = 0.12f;\n                    setFrontRearKnees(0.92f, 0.88f);\n                } else {\n                    body.airPose = RideAirPose::OllieDescent;\n                    body.frontFootLift = 0.04f;\n                    setFrontRearKnees(0.76f, 0.72f);\n                }\n            }\n        } else if (state_.phase == RidePhase::Landing) {\n            const float landingProgress = std::clamp(\n                state_.phaseSeconds / 0.38f,\n                0.0f,\n                1.0f\n            );\n            const float qualityWeight = state_.landingQuality == LandingQuality::Sketchy\n                ? 0.94f\n                : 0.62f;\n            body.landingCompression = std::sin(landingProgress * kPi) * qualityWeight;\n            body.footContact = RideFootContactState::Landed;\n            body.leftKneeFlex += body.landingCompression *\n                (state_.landingQuality == LandingQuality::Sketchy ? 0.92f : 0.72f);\n            body.rightKneeFlex += body.landingCompression *\n                (state_.landingQuality == LandingQuality::Sketchy ? 0.80f : 0.72f);\n            body.torsoLean += body.landingCompression * 0.10f;\n            body.armCounterbalance = state_.landingQuality == LandingQuality::Sketchy\n                ? stanceDirection * 0.60f\n                : stanceDirection * 0.16f;\n        }\n'''
replace_between(ride, skate_start, skate_end, skate_block, "stance-mirrored skateboard mechanics")

# Clamp procedural body values before publishing them to rendering/observer seams.
replace_exact(
    ride,
    "    }\n\n    state_.body = body;\n}\n\nTrickPhysicalIntent RideableMovementController::physicalIntentFor(\n",
    "    }\n\n"
    "    body.leftKneeFlex = std::clamp(body.leftKneeFlex, 0.0f, 1.35f);\n"
    "    body.rightKneeFlex = std::clamp(body.rightKneeFlex, 0.0f, 1.35f);\n"
    "    body.leftElbowFlex = std::clamp(body.leftElbowFlex, 0.0f, 1.35f);\n"
    "    body.rightElbowFlex = std::clamp(body.rightElbowFlex, 0.0f, 1.35f);\n"
    "    body.frontFootLift = std::clamp(body.frontFootLift, 0.0f, 0.42f);\n"
    "    body.rearLegDrive = std::clamp(body.rearLegDrive, 0.0f, 0.62f);\n"
    "    body.landingCompression = std::clamp(body.landingCompression, 0.0f, 1.0f);\n"
    "    state_.body = body;\n}\n\nTrickPhysicalIntent RideableMovementController::physicalIntentFor(\n",
    "body-mechanics safety clamps",
)

# Replace the old world-space knee kink with a stance-local two-segment solver.
leg_start = "    auto contactLeg = [&](float side,\n"
leg_end = "    auto contactArm = [&](float side,\n"
leg_block = '''    auto contactLeg = [&](float side,\n                          Vec3 target,\n                          float kneeFlex,\n                          float hipYaw,\n                          float footYaw) {\n        target.y -= groundContact.visualRootAbovePlayerBase;\n        const float flex = std::clamp(kneeFlex, 0.0f, 1.35f);\n        const float preloadDrop = ridingSkateboard\n            ? scene.rideable.body.preloadPoseWeight * 0.18f\n            : 0.0f;\n        const float landingDrop = mounted\n            ? scene.rideable.body.landingCompression * 0.15f\n            : 0.0f;\n        const Vec3 hip = rotateYawPoint(\n            {side * 0.23f,\n             1.17f - flex * 0.045f - preloadDrop - landingDrop,\n             0.0f},\n            hipYaw\n        );\n\n        // Bend the knee in the foot/stance plane rather than world -Z. This is\n        // the critical anti-marionette constraint: hips, knees and feet now\n        // agree on the board's local orientation.\n        const Vec3 bendForward = rotateYawPoint({0.0f, 0.0f, 1.0f}, footYaw);\n        const Vec3 outward = rotateYawPoint({side, 0.0f, 0.0f}, hipYaw);\n        const float bend = 0.08f + flex * 0.15f;\n        const Vec3 knee{\n            (hip.x + target.x) * 0.5f + bendForward.x * bend + outward.x * 0.045f,\n            (hip.y + target.y) * 0.5f + 0.12f - flex * 0.055f,\n            (hip.z + target.z) * 0.5f + bendForward.z * bend + outward.z * 0.045f\n        };\n        contactSegment(hip, knee, 0.27f);\n        contactSegment(knee, target, 0.25f);\n        const Vec3 footForward = rotateYawPoint({0.0f, 0.0f, 0.08f}, footYaw);\n        orientedLocalBox(\n            {target.x + footForward.x,\n             target.y + 0.06f,\n             target.z + footForward.z},\n            {0.32f, 0.14f, 0.50f},\n            footYaw,\n            Cyan\n        );\n    };\n\n'''
replace_between(renderer, leg_start, leg_end, leg_block, "stance-local knee/foot solver")

# Pelvis participates in preload/landing instead of hovering while knees fold.
replace_exact(
    renderer,
    "    orientedLocalBox(\n"
    "        {0.0f, 1.20f - scene.rideable.body.landingCompression * 0.12f, 0.0f},\n"
    "        {0.74f, 0.30f, 0.44f},\n",
    "    orientedLocalBox(\n"
    "        {0.0f,\n"
    "         1.20f - scene.rideable.body.preloadPoseWeight *\n"
    "             (ridingSkateboard ? 0.20f : 0.10f) -\n"
    "             scene.rideable.body.landingCompression * 0.18f,\n"
    "         0.0f},\n"
    "        {0.74f, 0.30f, 0.44f},\n",
    "pelvis preload/landing compression",
)

# Increase visible center-of-mass compression modestly; gameplay physics stay unchanged.
replace_exact(
    renderer,
    "    const float rideCompression = mounted\n"
    "        ? scene.rideable.body.preloadPoseWeight * 0.20f +\n"
    "            scene.rideable.body.landingCompression * 0.24f\n"
    "        : 0.0f;\n",
    "    const float rideCompression = mounted\n"
    "        ? scene.rideable.body.preloadPoseWeight *\n"
    "              (ridingSkateboard ? 0.27f : 0.18f) +\n"
    "            scene.rideable.body.landingCompression * 0.28f\n"
    "        : 0.0f;\n",
    "stronger visible center-of-mass compression",
)

# Extend deterministic assertions without changing ride-control semantics.
replace_exact(
    tests,
    "    assert(preloadController.state().body.pelvisYawRelativeToBoard < -1.2f);\n",
    "    assert(preloadController.state().body.pelvisYawRelativeToBoard < -1.2f);\n"
    "    assert(std::fabs(preloadController.state().body.pelvisYawRelativeToBoard) > 1.45f);\n"
    "    assert(preloadController.state().body.leftKneeFlex <= 1.35f);\n"
    "    assert(preloadController.state().body.rightKneeFlex <= 1.35f);\n",
    "stance mirror and clamp assertions",
)
replace_exact(
    tests,
    "    assert(boardController.state().body.footContact ==\n"
    "           RideFootContactState::ReleasedForTrick);\n",
    "    assert(boardController.state().body.footContact ==\n"
    "           RideFootContactState::ReleasedForTrick);\n"
    "    assert(boardController.state().body.leftKneeFlex >\n"
    "           boardController.state().body.rightKneeFlex);\n"
    "    assert(boardController.state().body.rearLegDrive > 0.50f);\n",
    "ollie pop front/rear sequencing assertions",
)

# Promote the passing v1 baseline into the v1.01 skate embodiment milestone.
replace_all(app, "v0.868-dev", "v1.01-dev", "app development version")
replace_all(app, "v0.868", "v1.01", "app version")
replace_all(app, "IMVU SOCIAL STACK", "SKATE EMBODIMENT PASS", "app milestone label")
replace_all(renderer, "v0.868", "v1.01", "renderer version label")

print("[HAKUI v1.01] SKATE EMBODIMENT PASS complete")
