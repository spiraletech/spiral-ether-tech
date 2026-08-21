# PROJECT HAKUI — Native Client v0.65-dev

[![Hakui Build and Test](https://github.com/spiraletech/spiral-ether-tech/actions/workflows/native-build.yml/badge.svg)](https://github.com/spiraletech/spiral-ether-tech/actions/workflows/native-build.yml)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-00599C.svg)](https://en.cppreference.com/w/cpp/20)
[![SDL3](https://img.shields.io/badge/SDL-3-17365D.svg)](https://www.libsdl.org/)

Hakui is a custom C++20 social/action world client built around a dependency-free **Spiral engine core**, deterministic gameplay/combat layers, an SDL3 GPU client, first-party avatar + interaction systems, and optional capability crystals.

The current milestone is architecture-first, but its dependency-free engine, gameplay, interaction layer, and avatar schema now have executable CTest contracts. GitHub Actions validates those contracts on Linux and Windows and compiles the native SDL3 client on Windows.

Planning is traceable through [`docs/PRODUCT_REQUIREMENTS.md`](docs/PRODUCT_REQUIREMENTS.md) and [`docs/IMPLEMENTATION_ROADMAP.md`](docs/IMPLEMENTATION_ROADMAP.md).

## Download

**SPIRAL OS: HAKUI ENGINE** Windows startup packs are published on the [Releases page](https://github.com/spiraletech/spiral-ether-tech/releases). Successful Windows workflow runs also provide a 30-day `spiral-os-hakui-engine-windows-x64` build artifact.

Extract the ZIP and run `SPIRAL-OS-HAKUI-ENGINE.exe`. The pack contains the executable, `SDL3.dll`, and `START_HERE.txt`.

## Quick start

Requirements: CMake 3.25 or newer and a C++20 compiler. The native client fetches the pinned SDL3 source during configuration.

Build and test the dependency-free layers:

```sh
cmake -S . -B build -DHAKUI_BUILD_NATIVE_CLIENT=OFF -DBUILD_TESTING=ON
cmake --build build --config Debug --parallel
ctest --test-dir build --build-config Debug --output-on-failure
```

Build the native client:

```sh
cmake -S . -B build -DHAKUI_BUILD_NATIVE_CLIENT=ON -DBUILD_TESTING=OFF
cmake --build build --config Release --target hakui --parallel
```

The resulting executable is `hakui` (`hakui.exe` on Windows). Run it from the selected build-configuration directory used by your generator.

## v0.65 DATA GRUNGE vertical slice

v0.65 turns the native proof into the first canonical HAKUI specimen. The world
is described as reusable primitives, semantic material roles, and descriptive
affordance volumes; deterministic systems interpret those descriptions, and the
SDL renderer presents their results.

The acceptance loop is:

```text
spawn → orient → walk/sprint/jump → ramp/platform traversal
      → sit/stand → anchored Fusion table session
      → enter spar → jab/cross/guard → hit/knockdown/recover → leave
      → fall through black space → respawn → pause/settings/resume
```

Current world identity includes powder-concrete masses, deliberate negative
space, a ramp and elevated unfinished gallery, CRT datum lines, sparse luminous
signage, a void couch, a fictional Fusion terminal/card table, a sculptural
monument, and a readable sparring datum. See
[`docs/DATA_GRUNGE_WORLD_GRAMMAR.md`](docs/DATA_GRUNGE_WORLD_GRAMMAR.md).

Combat is weapon-agnostic at its core. Unarmed is the only playable v0.65
discipline; sword and bow semantics are declared extension seams, not partially
implemented features. Hit decisions remain deterministic and renderer-free.

## Native architecture

```text
HAKUI CLIENT
├── SDL3 platform / input / GPU
├── debug 3D world renderer
├── semantic world description + affordances
├── deterministic movement / interaction / tabletop
├── generic combat simulation
│   └── playable unarmed discipline
├── Hakui-owned avatar rig
├── Hakui interaction layer
└── SpiralKernel heartbeat
    ├── Router Bus
    ├── Route Table
    ├── Monolith Ledger
    ├── StateStore
    ├── Ether Bus
    ├── Steam Engine
    ├── Pressure Rail
    ├── Mind + Coding Octopus Wheels
    ├── AUM 7x7 field
    ├── Crystal Grid
    └── Crystal Host
```

### Core law

```text
Mind Wheel   = policy
Coding Wheel = action
Core         = plumbing
```

Spiral Core does not depend on SDL, Cal3D, Boost, RapidXML, rendering, audio, or platform APIs.

## Native client

Hakui's visible proof slice currently contains:

- SDL3 GPU graphics pipeline
- depth-tested 3D rendering
- third-person-first camera roles for follow, interactions, combat framing, and future target/duel/director framing
- right-mouse orbit, wheel zoom, shoulder switch, reset, collision, and gamepad look
- persistent player world transform
- acceleration/deceleration, camera-relative WASD movement, sprint, jump, collision, ramps, elevated surfaces, black-space fall, and respawn
- camera-relative WASD steering
- procedural full-body avatar with idle/walk/run/jump/seated/combat poses and visible hit/knockdown response
- modular DATA GRUNGE specimen geometry and semantic 8-role material vocabulary
- contextual couch/table seating and table-anchored casino controls
- deterministic unarmed sparring with stance, jab, cross, guard, stamina, hit reaction, knockdown, and recovery
- procedural footsteps, interaction, casino, void, and combat audio cues
- AUM phase + Monolith event count + StateStore revision telemetry
- locomotion state changes published into Router Bus / StateStore
- a vendor-neutral Fusion Deck terminal with deterministic card and dice apps

Gameplay input remains immediate. Ether Bus is reserved for transitions that actually need its explicit transit/disembark gate.

## Spiral Core

### Router Bus

Typed live signal transport with Bus-owned monotonically increasing event IDs. Listener dispatch is snapshot-safe so subscribers may detach during an event.

### Route Table

Deterministic predicate routing:

```text
ordered rules
→ first match wins
→ no hidden fallback
→ route miss emits an error signal
```

### Monolith Ledger

Passive bounded audit ring. Default capacity: 1000 events.

**Monolith answers: what happened?**

### StateStore

Bus-fed typed reducer for canonical current state.

`State` signals may carry ordered key/value patches. Patches merge into the prior state using last-write-wins semantics, increment a revision counter, and retain the Bus signal ID that produced the latest state revision.

The store exposes a snapshot/restore boundary so future disk saves, networking, replay, or server persistence do not need to mutate gameplay objects directly.

**StateStore answers: what is true now?**

Example state keys already published by the native client include:

```text
client.status
client.version
avatar.bones
player.locomotion
wheel.mind.active
wheel.coding.active
```

Octopus wheel posture is committed to StateStore **only after Ether Bus transit completes and explicit Disembark occurs**.

### Ether Bus

Non-skippable transition state machine:

```text
Idle
→ Boarding
→ Transit
→ ReadyToDisembark
→ explicit Disembark
→ Idle
```

Hard transit floor: 2000 ms. Default: 2500 ms.

### Steam Engine / Pressure Rail

The current proof models 16 deterministic pistons, pressure, heat, redline, maintenance, condenser cooling, safety-valve venting, and an explicit steam drain. Pressure telemetry can travel beside Router Bus signals without becoming policy.

### AUM Field

```text
7 x 7 field
center = 3:3
A = emergence
U = sustain
M = return
3 sec / phase
9 sec / cycle
```

### Crystal Grid + Crystal Host

The Grid owns spatial/lifecycle relationships. The Host owns module memory and backend lifetime.

```text
CrystalHost owns module
      ↓
CrystalGrid references crystal
      ↓
A / U / M lifecycle
      ↓
Router Bus signals
```

Unmount detaches Grid references before module/backend memory is destroyed.

## Hakui interaction layer

Hakui no longer gives arbitrary world objects a mutable `PlayerState&` and lets them silently mutate the game.

The first-party `hakui_interaction` library uses a Bus-native protocol:

```text
PLAYER INPUT
    ↓
InteractionService
    ↓
interaction.request
    ↓
Interactable capability
    ↓
InteractionResult
    ├── exec/error event → Monolith
    └── State patch      → StateStore
```

Current interaction verbs include:

```text
Use
Enter
Mount
PickUp
Sit
Open
Talk
Trade
Play
Inspect
```

This is the common path intended for cars, BMX, skateboards, doors, TVs, consoles, NPCs, shops, EtherTech tables, furniture, and future world objects.

Interaction targets use weak references so expired world objects cannot leave dangling interaction pointers.

`tests/hakui/InteractionSpec.cpp` is an executable CTest contract for request logging, verb validation, state patching, missing targets, and safe object expiry.

## Avatar architecture

Hakui owns its canonical humanoid schema through `HakuiSkeleton`.

It is a plain first-party C++ data model containing:

- bone names
- parent indices
- stable attachment slots

It contains **no SDL or Cal3D types**.

Current attachment slots include body, head, hair, face, neck, torso, back, waist, hands, feet, skateboard, BMX, and vehicle seating.

## Optional IMVU-Cal3D crystal

IMVU's public Cal3D fork is not linked directly into the Hakui executable.

When explicitly enabled, the path is:

```text
HakuiSkeleton
      ↓
ImvuCal3DBackend
      ↓
ImvuCal3DModule
      ↓
crystal.imvu_skeleton
      ↓
CrystalHost / AUM lifecycle
```

Only a minimal skeletal transform subset is selected. Loader/XML/saver/model/renderer sources are excluded, and the fork's `USE_CAL3D_WITH_CPP_11` path is enabled so the current skeletal slice does not require Boost or RapidXML.

Cal3D remains **OFF by default**.

## Build targets

```text
spiral_core
hakui_avatar_rig
hakui_interaction
hakui_gameplay
hakui_tabletop
hakui
```

Optional:

```text
hakui_cal3d_skeleton
spiral_imvu_cal3d_backend
spiral_logic_spec
hakui_interaction_spec
hakui_avatar_rig_spec
hakui_gameplay_spec
hakui_tabletop_spec
imvu_cal3d_backend_spec
```

## Controls

```text
W A S D       move on foot
Shift         sprint
1             on-foot mode
2             skateboard mode
3             BMX mode
4             car mode
T             power/use terminal; roll dice when online
G             open the 52-card table suite
B             begin a round with 25 virtual credits
H             hit
J             stand
I             inspect the terminal
Esc           quit
```

The tabletop suite uses fictional virtual credits only. It has no real-money purchase, deposit, cash-out, marketplace, or external gambling integration. Terminal models—Nebula Tower, Orchard Glass, and Fusion Deck—are original in-world identities and do not represent real hardware companies.

Only on-foot physics is implemented in the visible proof slice; the other locomotion states are scaffolds.

## Dependency firewall

CMake scans first-party Spiral Core, avatar-rig, gameplay, interaction, and tabletop sources and rejects direct SDL3, Cal3D, Boost, or RapidXML includes in those layers. The explicit optional crystal backend directory is the deliberate legacy-runtime exception.

## Vocabulary map

Hakui keeps its project-specific language, while each name maps to a conventional engineering role:

| Hakui term | Conventional role |
| --- | --- |
| Router Bus | Typed in-process event bus |
| Route Table | Ordered predicate router |
| Monolith Ledger | Bounded event/audit log |
| StateStore | Reducer-backed canonical state |
| Ether Bus | Timed transition state machine |
| Steam Engine / Pressure Rail | Deterministic resource telemetry |
| AUM Field | Three-phase lifecycle scheduler |
| Crystal Grid / Host | Capability registry and lifetime owner |
| Octopus Wheels | Policy and action selectors |

## Validation

Every push and every pull request runs the dependency-free test suite on Linux and Windows. A separate Windows job compiles the SDL3 native client. The core test configuration is:

```text
HAKUI_BUILD_NATIVE_CLIENT=OFF
HAKUI_ENABLE_IMVU_CAL3D=OFF
BUILD_TESTING=ON
```

CTest currently covers Spiral engine invariants, interaction behavior, and avatar-rig integrity. Assertions remain enabled for spec executables even when a release-style configuration is selected.

Validation order:

1. static architecture audit
2. Spiral Core
3. StateStore + interaction contracts
4. first-party Hakui avatar rig
5. native Hakui client
6. optional IMVU-Cal3D crystal backend
7. only then GPU-skinned avatar integration

See `docs/SPIRAL_ENGINE_CANON.md` for the architectural laws and `THIRD_PARTY.md` for license notes.
