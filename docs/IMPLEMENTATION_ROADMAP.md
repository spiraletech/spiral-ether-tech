# Hakui Implementation Roadmap

This roadmap turns `PRODUCT_REQUIREMENTS.md` into independently verifiable slices. A milestone closes only when its tests and manual checks are complete.

## M0 — Validation foundation

- Register first-party specifications with CTest.
- Run core tests on Linux and Windows for every push and pull request.
- Compile the native client in Windows CI.
- Keep assertions enabled in specification executables.

Exit signal: all automated jobs are green.

## M1 — Playable movement slice

- Move deterministic on-foot rules out of the SDL application shell.
- Normalize diagonal input and cap simulation delta.
- Implement sprint stamina consumption and recovery.
- Add gameplay movement specifications.
- Publish throttled transform and stamina patches into StateStore.

Current state: controller and tests implemented; StateStore publication remains.

## M2 — Interaction proof

- Create one visible door or Ether Gate target.
- Present available verbs in a minimal in-world prompt.
- Route Enter/Open/Inspect through `InteractionService`.
- Commit returned world patches to StateStore.
- Prove safe target expiry and missing-target errors.

Exit signal: a player can approach, inspect, and operate one object through the common interaction path.

## M3 — Locomotion controllers

- Add distinct skateboard, BMX, and vehicle controller interfaces.
- Implement mount/dismount through explicit interactions.
- Use Ether transitions only where the design requires a gated transition.
- Add controller-specific physics contracts.

Exit signal: no selectable mode is a label-only scaffold.

## M3.5 — Tabletop terminals

- Add a deterministic traditional 52-card deck and bounded dice roller.
- Implement a virtual-credit blackjack rules engine.
- Route terminal power, inspection, cards, and dice through world interactions.
- Use original terminal models and presentation assets.
- Keep all credits fictional and non-purchasable.

Current state: domain logic, terminal interaction, and specs implemented; visible terminal geometry and card-table UI remain.

## M4 — Avatar and animation

- Replace the cuboid proxy with a first-party test avatar.
- Map movement state to an animation-state interface.
- Validate attachment slots visually and structurally.
- Keep optional Cal3D types behind the crystal backend.

Current state: a procedural debug gait now drives visible limbs from movement
state, with smoothed idle/walk/sprint blending. A first-party skinned mesh and
attachment visualization remain.

Exit signal: on-foot movement drives a visible rig without breaking the dependency firewall.

## M4.5 — v0.65 DATA GRUNGE specimen

- Replace the generic room with modular powder-concrete/industrial geometry,
  CRT accents, negative space, ramps, platforms, furniture, and monuments.
- Describe rideable, grindable, transition, launch, landing, seating, casino,
  terminal, combat, spectator, respawn, and void affordances without placing
  system rules in the world layer.
- Complete the movement → seating → table → sparring → void → pause loop.
- Add third-person camera roles for follow, interaction, combat, and future
  target/duel/spectator/director framing.
- Add a weapon-agnostic combat simulation with unarmed as the only playable
  discipline and disabled sword/bow extension seams.
- Present deterministic combat events through full-body poses, HUD, sound, hit
  reaction, knockdown, and recovery.

Current state: implemented locally on `codex/data-grunge-v0.65`; native/manual
visual QA and remote CI remain before merge authorization.

Exit signal: launching the executable immediately reads as HAKUI, and the full
acceptance path remains coherent in one specimen environment.

## M4.6 — v0.7 embodiment pass

- Move camera yaw, pitch, distance, shoulder, reset, and sensitivity rules into
  a deterministic camera target independent of SDL/GPU code.
- Repair native RMB relative-mouse capture and guarantee release on button-up,
  focus loss, pause, and shutdown.
- Resolve couch, table, and combat entry from authored affordance anchors.
- Keep the sparring dummy visible and the FightZone physically reachable.
- Add distinct skateboard/BMX motion profiles and attached procedural models.
- Preserve on-foot animation and explicitly defer Car instead of presenting an
  invisible implementation.
- Complete one native acceptance session covering camera, movement, seating,
  tabletop, combat, ride modes, void recovery, pause, and a setting change.

Current state: implemented and verified locally on
`codex/embodiment-v0.7`; seven deterministic CTests and the native client build
pass. Window-targeted native smoke checks completed locally. Remote CI, review,
packaging, and publication remain unauthorized.

Exit signal: every system advertised in v0.7 is visible, controllable, and
understandable; intentionally incomplete systems are labeled honestly.

## M5 — World persistence and replay

- Define a versioned StateStore snapshot schema.
- Save and restore player transform and operated world objects.
- Record enough ordered inputs/events for a deterministic replay proof.
- Reject incompatible snapshots with an actionable error.

Exit signal: restart restores the same proof world and a short recorded session can be replayed.

## M6 — Release candidate

- Select and add the first-party license.
- Add a native smoke-test checklist and release packaging.
- Capture current screenshots/video.
- Document supported hardware, controls, and known limitations.
- Require green CI before tagging.

Exit signal: a new contributor can clone, build, test, run, and understand the proof without private context.
