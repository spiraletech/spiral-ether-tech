# Hakui Product Requirements

Status: working specification through the local v0.81 cleanup and Expert AI observer pass.

Each requirement has a stable identifier so code, tests, issues, and release notes can refer to the same contract. A requirement is complete only when its acceptance criteria are automated or explicitly marked as a manual visual/audio check.

## Product goal

Hakui v1.0 is a responsive native social/action-world proof in which a player can enter a world, move a persistent avatar, interact with typed world capabilities, and observe those actions through the deterministic Spiral event and state layers.

## Experience requirements

### HK-EXP-001 — Enter a live world

The client shall create an SDL3 window, initialize a supported GPU backend, construct the canonical avatar rig, and enter the world loop without optional capability backends.

Acceptance criteria:

- The default Windows build produces `hakui.exe`.
- Startup failure returns a non-success result and emits a useful SDL error.
- The client can run with `HAKUI_ENABLE_IMVU_CAL3D=OFF`.

### HK-EXP-002 — Responsive on-foot movement

The player shall move with normalized WASD input, face the movement direction, sprint while stamina is available, and remain stable after a frame stall.

Acceptance criteria:

- Diagonal input is not faster than axial input.
- Walk speed is 3.25 world units per second.
- Sprint speed is 5.75 world units per second.
- Simulation delta is capped at 100 ms.
- Sprint drains stamina and rest recovers it within the range `[0, 100]`.
- Invalid numeric input cannot introduce non-finite player state.

Automated by: `hakui.gameplay_movement`.

### HK-EXP-003 — Explicit locomotion modes

The player shall be able to select on-foot, skateboard, BMX, and vehicle modes. A mode may not silently reuse another mode's physics.

Acceptance criteria:

- Mode changes publish `locomotion.changed` and update `player.locomotion`.
- On-foot, skateboard, and BMX use distinct grounded motion profiles.
- Skateboard and BMX cannot silently reuse on-foot jump behavior.
- Car remains immobile and is visibly identified as deferred until it has a
  physical representation and controller.

Automated in part by: `hakui.gameplay_movement`.

### HK-EXP-004 — Typed world interaction

World objects shall expose explicit verbs and return an interaction result rather than mutating player state through an unrestricted reference.

Acceptance criteria:

- Requests, successful execution, denial, and missing targets are observable events.
- State changes enter the canonical StateStore only through a returned state patch.
- Expired world objects cannot leave dangling targets.

Automated by: `hakui.interaction`.

### HK-EXP-005 — Visible proof slice

The native client shall render a depth-tested world floor, a player proxy, and live telemetry sufficient to prove that world, state, and Spiral systems are advancing.

Acceptance criteria:

- A manual smoke-test checklist verifies the viewport and controls.
- The title or debug overlay exposes AUM phase, event count, StateStore revision, and player position.
- A release artifact includes at least one current screenshot or short capture.

### HK-EXP-006 — Fictional tabletop suite

The world shall support deterministic 52-card and dice activities using fictional virtual credits only.

Acceptance criteria:

- A standard deck contains exactly 52 unique suit/rank combinations and cannot draw past exhaustion.
- A supplied random seed reproduces the same shuffle and dice sequence.
- Dice requests validate count and side limits before rolling.
- Blackjack evaluates soft aces, validates wagers, deals without replacement, applies dealer draw rules, and settles wins, losses, pushes, and natural blackjack.
- Credits cannot be purchased, deposited, transferred, redeemed, or cashed out.

Automated by: `hakui.tabletop`.

### HK-EXP-007 — Usable world terminals

The player shall be able to power on, inspect, and launch tabletop applications from original, vendor-neutral terminals.

Acceptance criteria:

- Terminal actions use the common `InteractionService` request/result path.
- Power, selected app, and dice results publish StateStore patches.
- Terminal models use original in-world names and visual identities rather than real-company names, logos, or trade dress.
- Humor may be absurd and satirical but may not reproduce protected characters, dialogue, sketches, or a specific show's distinctive presentation.

Automated by: `hakui.tabletop`.

### HK-EXP-008 — Recognizable DATA GRUNGE world

The default specimen shall be recognizable as HAKUI rather than a generic C++
test room. It shall use modular architecture, strong silhouettes, deliberate
negative space, restrained industrial materials, sparse CRT accents, and
legible interaction landmarks.

Acceptance criteria:

- Layout is described with reusable `WorldPrimitive` records.
- Materials are semantic roles that a later renderer/editor may reinterpret.
- The specimen contains a plaza, ramp, elevated platform, void boundary,
  seating, casino/terminal anchor, sparring datum, and sculptural monument.
- Rendering consumes world description and never owns deterministic rules.

Automated in part by: `hakui.gameplay_movement`. Visual identity remains a
manual smoke check.

### HK-EXP-009 — Integrated specimen loop

One coherent executable session shall support movement, traversal, seating,
tabletop interaction, sparring, black-space recovery, pause, and settings.

Acceptance path:

`spawn → orient → walk → sprint → jump → traverse → sit → stand → table session
→ spar → fall → respawn → pause → change setting → resume`.

### HK-EXP-010 — Extensible third-person combat proof

The player shall be able to enter a semantic fight zone and complete a minimal
unarmed exchange without coupling hit decisions to rendering.

Acceptance criteria:

- Combat state is separate from equipped discipline behavior.
- Shared state owns intent, stamina, targeting, damage/hit results,
  interruption, knockdown, and recovery.
- Unarmed supports stance, jab, cross, guard, receive-hit, knockdown, recovery,
  and exit.
- Sword and bow semantics remain disabled extension points through v0.8.
- Damage events identify source, target, discipline, semantic, amount, impact,
  stagger, knockdown potential, and result.
- The camera and procedural avatars remain third-person and full-body readable.

Automated by: `hakui.combat`.

### HK-EXP-011 — Reliable third-person camera control

The avatar shall remain the visual center while native input provides a
controllable orbit camera.

Acceptance criteria:

- RMB begins relative-mouse orbit and button-up stops it.
- Yaw wraps safely and pitch remains within the authored limits.
- Wheel zoom, reset, and sensitivity adjustment remain usable; shoulder
  switching has no player-facing binding or prompt.
- Focus loss, pause, and shutdown release relative-mouse capture.
- Resuming never leaves the cursor captured unless the player presses RMB again.
- Camera rules remain independently testable without SDL or a GPU.

Automated in part by: `hakui.camera`. Native input/capture is a smoke check.

### HK-EXP-012 — Visible personal mobility embodiment

Selectable personal-mobility modes shall have a physical representation and a
distinct gameplay response.

Acceptance criteria:

- Skateboard mode attaches a grounded deck, trucks, and four readable wheels.
- BMX mode attaches a frame, wheels, fork, handlebars, seat, and crank region.
- Avatar height and pose adapt to the active ride.
- Movement turns the avatar and ride together and advances wheel presentation.
- Leaving a mode removes its mesh without stale state.
- Ride modes dismount to on-foot before seating or sparring.
- v0.75 provides a focused skateboard/BMX grammar: hop, manual, grind, air,
  landing quality, bail, and one or two discipline-specific air tricks.
- A full trick encyclopedia and advanced transition physics remain out of scope.

Automated in part by: `hakui.gameplay_movement` and
`hakui.rideable_movement`. Mesh readability is a native visual smoke check.

### HK-EXP-013 — Semantic multi-device control

Hardware shall express intent through a platform-neutral action layer before an
active discipline interprets gameplay meaning.

Acceptance criteria:

- Gameplay consumers do not read SDL scancodes or gamepad button names.
- The same jump action becomes jump, ollie, or bunny hop by active discipline.
- Keyboard/mouse and SDL gamepads can hot-swap without restarting the client.
- The last active device selects prompt presentation.
- Controller disconnect clears stale ride input and never corrupts camera state.
- Public skateboard/BMX activation without a gamepad produces a clear status
  message; a developer-only keyboard fallback remains available.
- BMX front/rear axle and grip placement obey a tested local `+Z`-forward
  convention.

Automated in part by: `hakui.input`, `hakui.avatar_rig`,
`hakui.gameplay_movement`, `hakui.rideable_movement`, and `hakui.combat`.

### HK-EXP-014 — Read-only Expert AI inspection contract

The running native client shall export a compact, versioned diagnostic bundle
without granting the observer gameplay, process, source-control, or publication
authority.

Acceptance criteria:

- F12 requests one timestamped bundle beneath `HAKUI-OBSERVE` while runtime
  simulation continues normally.
- The manifest references parseable build, world, entity, input, camera, and
  runtime JSON plus a bounded log, top-down semantic SVG, doctrine, and current
  rendered PNG frame.
- Stable object/entity identifiers, affordances, transforms, attachment
  semantics, current interaction intent, and input prompts are represented.
- The deterministic exporter compiles and tests without SDL or a GPU; native
  frame capture exists only at the platform/presentation boundary.
- The observer cannot inject input, mutate world state, execute commands, or
  perform source-control/network operations.

Automated in part by: `hakui.observer`. Current-frame fidelity remains a native
visual smoke check.

## Engine requirements

### HK-ENG-001 — Dependency firewall

Spiral Core, avatar schema, gameplay rules, and interaction rules shall remain free of SDL, rendering, Cal3D, Boost, RapidXML, audio, and platform APIs.

Acceptance criteria:

- CMake configuration fails when a forbidden include enters a protected first-party layer.
- Protected targets configure and test with the native client disabled.

### HK-ENG-002 — Deterministic canonical state

StateStore shall be the authoritative shared-state reducer. Accepted patches increment a revision and retain the originating signal identifier.

Automated by: `spiral.logic`.

### HK-ENG-003 — Observable routing

Routing shall be ordered, first-match-wins, and have no hidden fallback. Route misses shall produce an error signal.

Automated by: `spiral.logic`.

### HK-ENG-004 — Explicit transitions

Transitions that use Ether Bus shall pass through Boarding, Transit, ReadyToDisembark, and explicit Disembark. They may not commit transition state early.

Automated by: `spiral.logic`.

### HK-ENG-005 — Capability lifetime safety

CrystalHost shall own capability memory, and CrystalGrid references shall detach before owned capability memory is destroyed.

Automated by: `spiral.logic` and, when enabled, `spiral.imvu_cal3d_backend`.

### HK-ENG-006 — World/simulation/presentation separation

The world shall advertise descriptive affordances while gameplay systems decide
their meaning. Presentation may consume gameplay state/events but may not decide
movement collision, casino outcomes, attacks, hits, damage, or respawn.

Required flow:

`WORLD DESCRIPTION → GAMEPLAY/SIMULATION → PRESENTATION → SDL RENDERER`.

Automated in part by dependency firewalls plus `hakui.gameplay_movement`,
`hakui.camera`, `hakui.combat`, and `hakui.tabletop`.

## Quality requirements

### HK-QLT-001 — Automated validation

Every push and pull request shall run dependency-free tests on Linux and Windows. Windows CI shall also compile the native SDL3 client.

### HK-QLT-002 — Warning-clean first-party code

First-party targets shall compile with high warning levels (`/W4 /permissive-` or `-Wall -Wextra -Wpedantic`). Test assertions shall remain active in every configuration.

### HK-QLT-003 — Reproducible dependencies

Third-party source dependencies shall use pinned releases or commits and be documented in `DEPENDENCIES.txt` and `THIRD_PARTY.md`.

### HK-QLT-004 — Public release hygiene

Before a public v1.0 release, the repository shall include an owner-selected license, build instructions, current media, and a passing build badge.

## Requirement states

| Requirement | State | Evidence |
| --- | --- | --- |
| HK-EXP-001 | Implemented, awaiting CI | `HakuiApp`, Windows native build job |
| HK-EXP-002 | Implemented, awaiting CI | `PlayerMovementController`, gameplay spec |
| HK-EXP-003 | Implemented in part locally | On-foot, skateboard, and BMX are distinct; car deferred |
| HK-EXP-004 | Implemented, awaiting CI | `InteractionService`, interaction spec |
| HK-EXP-005 | Partial | Debug renderer exists; smoke checklist/media remain |
| HK-EXP-006 | Implemented, awaiting CI | Card deck, dice, blackjack, tabletop spec |
| HK-EXP-007 | Implemented, awaiting CI | Game terminal, interaction routing, tabletop spec |
| HK-EXP-008 | Implemented locally | `BlackRoom`, `WorldGeometry`, gameplay spec |
| HK-EXP-009 | Implemented locally | Native v0.8 control/embodiment acceptance loop; remote CI unauthorized |
| HK-EXP-010 | Implemented locally | `CombatSimulation`, deterministic footwork, combat spec, native spar presentation |
| HK-EXP-011 | Implemented locally | `ThirdPersonCameraRig`, camera spec, native capture smoke |
| HK-EXP-012 | Implemented locally | `RideableMovementController`, rideable spec, distinct procedural models |
| HK-EXP-013 | Implemented locally | `HakuiInput`, `SdlInputBridge`, discipline interpreter, semantic HUD prompts, ride attachment spec |
| HK-EXP-014 | Implemented locally | `ExpertObserver`, native F12 frame capture, observer spec, versioned read-only bundle |
| HK-ENG-001–005 | Implemented, awaiting CI | Firewall and Spiral specs |
| HK-ENG-006 | Implemented locally | Semantic affordances and dependency-free combat/gameplay targets |
| HK-QLT-001–003 | Implemented, awaiting CI | CMake, workflow, dependency manifest |
| HK-QLT-004 | Partial | License and current media require owner input |

## Open owner decisions

- First-party source license: MIT, Apache-2.0, or proprietary/no-license.
- First shippable authored avatar asset within the established DATA GRUNGE direction.
- Whether v1.0 networking is local-loopback proof, client/server authoritative, or deferred.
- Supported release platforms beyond Windows.
