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

## M4.7 — v0.75 expressive movement pass

- Add a shared deterministic ride state for grounded, airborne, grinding,
  manual, landing, crash, momentum, balance, and landing quality.
- Let skateboard and BMX interpret authored `Grindable`, `Launch`, and
  `ManualZone` volumes with distinct verbs instead of duplicating controllers.
- Add ollie/bunny-hop, discipline-specific manual/grind, focused air tricks,
  landing quality, bail, and a small combo seed.
- Turn the Black Room into a connected movement playground with a manual strip,
  low rail, transfer kicker, gallery ramp, and elevated edge.
- Make BMX steering readable through an explicit front wheel, fork, stem,
  handlebars, grips, frame, seat, and crank silhouette.
- Add deterministic boxing footwork and improve stance, strike commitment,
  impact, stagger, guard, knockdown, and recovery presentation.
- Preserve the v0.7 camera rig and its deterministic camera tests unchanged.

Current state: implemented locally on `codex/expressive-movement-v0.75`; eight
deterministic CTests and the native client compile locally. Remote operations
remain unauthorized.

Exit signal: a player can connect a short board/BMX movement line, read the
resulting trick and landing feedback, then dismount into kinetic unarmed
sparring without losing camera fluency.

## M4.8 — v0.8 control nervous system

- Terminate SDL controls at one native adapter and publish platform-neutral
  action/axis frames to gameplay consumers.
- Interpret the shared intent grammar through on-foot, skateboard, BMX, boxing,
  and seated contexts without changing deterministic movement rules.
- Track the last active device, hot-swap prompts, and recover safely after a
  controller disconnect.
- Require a connected controller to activate advanced public ride modes while
  retaining an explicitly marked developer keyboard fallback.
- Resolve prompts from semantic actions instead of storing physical keys in
  world-interaction text.
- Define ride attachment anchors and rebuild BMX front steering around a stable
  `+Z`-forward local convention.

Current state: implemented locally on `codex/control-nervous-system-v0.8`; nine
deterministic CTests and the native SDL client compile locally. Runtime visual
and control-feel checks remain part of the local acceptance pass. Remote
operations remain unauthorized.

Exit signal: keyboard/mouse and hot-swapped SDL gamepads speak one coherent
intent language, advanced rides fail clearly without a controller, and the BMX
front assembly stays visibly and structurally forward while steering.

## M4.81 — v0.81 cleanup + Expert AI eyes

- Remove shoulder switching from current player bindings and prompts while
  preserving orbit, zoom, camera collision, ride framing, and combat framing.
- Resolve visual avatar height through named embodiment ground-contact profiles.
- Route the Fusion table through universal Interact and one contextual action.
- Export a versioned, read-only F12 inspection bundle containing build, world,
  entity, input, camera, runtime, map, current-frame, log, and doctrine views.
- Keep semantic export deterministic and SDL-free; isolate Win32 frame capture
  at the native presentation boundary.

Current state: implemented locally on `codex/cleanup-observer-v0.81`; ten
deterministic CTests and the native SDL client compile locally. Remote
operations remain unauthorized.

Exit signal: HAKUI stands correctly on authored contact surfaces, interaction
controls are quieter, and one F12 snapshot lets an external expert compare
semantic truth with the actual rendered frame.

## M4.82 — v0.82 canonical controller overhaul

- Give skateboard and BMX one semantic ride grammar: South pop, North grind,
  LT balance, RT propulsion, LB/RB spin, West style, and East dismount.
- Centralize ride camera/trick deadzones, activation/release thresholds, timing,
  gesture classification, and ownership in a deterministic SDL-free layer.
- Reset right-stick ownership on gesture completion, pause, disconnect,
  dismount, locomotion switch, and void respawn.
- Resolve prompts for PlayStation-, Xbox-, and generic SDL controller families
  while gameplay continues to consume semantic actions only.
- Validate grind affordance, proximity, speed, approach alignment, ride state,
  and attachment opportunity before entering a grind.
- Extend Expert Observer input snapshots with complete ride-control diagnostics
  and tuning values.

Current state: implemented locally on `codex/controller-overhaul-v0.82`; ten
deterministic CTests and the native SDL client compile locally. Remote
operations remain unauthorized.

Exit signal: skateboard and BMX feel like distinct machines speaking the same
controller language, right-stick ownership can never become stuck, and the
entire interpretation path is visible in one read-only snapshot.

## M4.83 — v0.83 pop → flick correction

- Emit ride pop immediately on South press; never require the button to remain
  held for trick recognition.
- Arm a brief deterministic flick window only after gameplay confirms the pop
  made the skateboard or BMX airborne.
- Consume the first decisive eight-way flick as the primary trick intent, then
  return the right stick to the camera.
- Treat no-flick, grounded-flick, and late-flick paths as normal pop/camera
  behavior rather than accidental tricks.
- Close the trick window on recognition, timeout, landing, bail, pause,
  dismount, respawn, mode switch, or controller disconnect.
- Keep North/Triangle grind, LT balance, RT propulsion, BMX attachment geometry,
  and the rest of the v0.82 grammar unchanged.
- Replace obsolete held-button diagnostics and prompts with sequential pop,
  airborne-window, flick, trick-intent, and camera-ownership observations.

Current state: implemented locally on `codex/pop-flick-correction-v0.83`; ten
deterministic CTests and the native SDL client compile locally. Remote
operations remain unauthorized.

Exit signal: the readable physical rhythm is `POP → AIR → FLICK → LAND`, with
camera ownership restored cleanly on every success, timeout, and interruption.

## M4.84 — v0.84 ride physics + embodiment

- Add a capped South-button preload whose release emits the existing semantic
  pop, then arm the right-stick trick window only after confirmed airtime.
- Give the minimum skateboard and BMX vocabulary deterministic rotation
  channels, axes, directions, targets, angular speeds, airtime requirements,
  and body-assist metadata.
- Integrate rideable orientation in the gameplay layer and evaluate landing
  completion, orientation, contact, impact, balance, and surface normal without
  renderer-authored success or magic orientation snapping.
- Classify clean, sketchy, failed, and bail outcomes with explicit reasons and
  a visible procedural tumble/recovery path.
- Author BoardRoot/BikeRoot attachment hierarchies with deck/truck/foot and
  steering/grip/crank/pedal/seat semantics; drive procedural rider limbs from
  those local anchors.
- Extend the read-only Expert Observer with ride-physics and embodiment-contact
  diagnostics, and add minimal procedural physical-readability cues.

Current state: implemented locally on `codex/ride-physics-embodiment-v0.84`;
deterministic specifications and the native SDL client compile locally. Remote
operations remain unauthorized.

Exit signal: trick intent evolves as visible machine rotation, completed motion
lands, unfinished motion bails, BMX hands stay on steering-derived grips, and
feet visibly relate to pedals or deck contacts.

## M4.85 — v0.85 body mechanics + social embodiment

- Add deterministic regular/goofy skate-stance semantics with sideways pelvis,
  counter-rotated torso, look compensation, dedicated ride idle/rolling pose,
  and body preload driven by the existing pop-preload value.
- Describe ollie and BMX hop phases, minimum trick-specific pose differences,
  foot release/reacquisition, clean/sketchy landing compression, and bail loss
  of stance without moving gameplay success into SDL presentation.
- Preserve BMX grip/pedal attachment truth while adding bent limbs, torso
  commitment, steering response, and pull/tuck/descent mechanics.
- Generalize furniture interaction into reservable local-space seat anchors
  with stable IDs, occupancy, pose profiles, resolved transforms, and release.
- Give the VOID COUCH explicit left/right slots and preserve stable seated
  alignment and camera framing.
- Extend Expert Observer with requested body and social seating diagnostics.

Current state: implemented locally on
`codex/body-mechanics-social-seating-v0.85`; dependency-free specifications and
the native SDL client compile locally. Remote operations remain unauthorized.

Exit signal: the skater reads as sideways and connected, preload/air/landing
mechanics are visible, BMX limbs explain the machine, and couch seating chooses
a reservable side slot instead of the center.

## M4.86 — v0.86 social chat + interaction overhaul

- Repair left/right couch selection and presentation while preserving the same
  local-slot-to-world reservation and pelvis-alignment pipeline on both sides.
- Add a dependency-free social-language layer for bounded local/system history,
  Unicode-aware editing, bubble lifetime, speech intent, and gesture selection.
- Give SDL text input exclusive `ChatInput` ownership so typed gameplay and
  developer keys cannot leak into avatar, ride, combat, camera, or interaction.
- Render camera-facing avatar bubbles, a compact lower-edge entry surface, and
  subtle social gestures beneath all physical animation authorities.
- Extend Expert Observer with every couch slot plus complete chat, source,
  intent, bubble-anchor, lifetime, and gesture diagnostics.

Current state: implemented locally on `codex/social-chat-interaction-v0.86`;
dependency-free specifications and the native SDL client compile locally.
Remote operations remain unauthorized.

Exit signal: the player can chat while roaming or seated, typing never moves the
avatar, local speech becomes an avatar-attached world event, system text remains
distinct, both couch slots survive repeated cycles, and all v0.85 systems retain
authority.

## M4.861 — v0.861 social bubble visual overhaul

- Keep the v0.86 social simulation and input-ownership boundaries unchanged.
- Make active human speech a world-space `ChatBubbleAnchor` presentation rather
  than a dominant opaque screen-space slab.
- Add reusable glass material, rounded primitive silhouette, tapered tail,
  proportional sentence-case glyphs, word wrapping, line/width bounds,
  distance scaling, and phased fade/motion rules.
- Replace text entry with a compact translucent capsule and keep system
  notifications distinct from avatar speech.
- Export rendered bubble world/screen position, scale, alpha, dimensions, line
  count, style/phase, camera distance, and anchor error through Expert Observer.
- Retain WeatherDX, Saelis, NPC, and SystemAI profile seams without implementing
  weather, AI dialogue, networking, or speaker-specific effects.

Current state: implemented locally on `codex/social-bubble-visual-v0.861`;
deterministic presentation tests and the native glass pipeline compile locally.
Remote operations remain unauthorized.

Exit signal: `hey lol` reads as lightweight speech floating with the avatar,
not as a black debug subtitle covering the world.

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
