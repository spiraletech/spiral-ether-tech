# PROJECT HAKUI — Native Client v0.4

Hakui is a custom C++20 social/action world client built around a dependency-free **Spiral engine core**, an SDL3 GPU client layer, first-party avatar + interaction systems, and optional capability crystals.

The current branch is **architecture-first and intentionally unvalidated**. Automatic CI is disabled; no source-level spec should be described as passing until the manual validation stage is deliberately triggered.

## v0.4 architecture milestone

```text
HAKUI CLIENT
├── SDL3 platform / input / GPU
├── debug 3D world renderer
├── immediate gameplay locomotion
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
- high-angle follow camera
- persistent player world transform
- WASD on-foot movement
- Shift sprint
- grey cuboid humanoid debug proxy
- large world floor
- AUM phase + Monolith event count + StateStore revision telemetry
- locomotion state changes published into Router Bus / StateStore

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

`tests/hakui/InteractionSpec.cpp` is an **unrun** source contract for request logging, verb validation, state patching, missing targets, and safe object expiry.

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
hakui
```

Optional:

```text
hakui_cal3d_skeleton
spiral_imvu_cal3d_backend
spiral_logic_spec
hakui_interaction_spec
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
Esc           quit
```

Only on-foot physics is implemented in the visible proof slice; the other locomotion states are scaffolds.

## Dependency firewall

CMake scans first-party Spiral Core, avatar-rig, and interaction sources and rejects direct SDL3, Cal3D, Boost, or RapidXML includes in those layers. The explicit optional crystal backend directory is the deliberate legacy-runtime exception.

## Validation policy

Automatic GitHub Actions runs are paused.

The manual core workflow is configured to isolate Spiral first:

```text
HAKUI_BUILD_NATIVE_CLIENT=OFF
HAKUI_ENABLE_IMVU_CAL3D=OFF
HAKUI_ENABLE_SPIRAL_LOGIC_SPECS=ON
```

The interaction layer and optional IMVU backend each have separate, currently unrun spec targets.

Validation order:

1. static architecture audit
2. Spiral Core
3. StateStore + interaction contracts
4. first-party Hakui avatar rig
5. native Hakui client
6. optional IMVU-Cal3D crystal backend
7. only then GPU-skinned avatar integration

See `docs/SPIRAL_ENGINE_CANON.md` for the architectural laws and `THIRD_PARTY.md` for license notes.
