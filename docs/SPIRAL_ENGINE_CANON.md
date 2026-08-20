# SPIRAL ENGINE CANON — Hakui Integration

Status: **architecture wired / intentionally unrun**

This document freezes the recovered Spiral OS machinery and the dependency boundaries used by Project Hakui.

## 1. Two-Wheel Axiom

There are exactly two independent eight-notch wheels.

### Mind Wheel = Policy

- eight ordered notch slots
- one active notch at a time
- decides policy, including whether stop may be allowed
- never performs the action itself

### Coding Wheel = Action

- eight ordered notch slots
- one active notch at a time
- represents/dispatches action
- never decides whether the system should stop

### Orthogonality law

The wheels do not command each other.

```text
MIND WHEEL ──┐
             ├── CORE / PLUMBING
CODING WHEEL ┘
```

They meet only through core state/routing.

The original notch labels/order were not recovered from the available logs, so engine code intentionally exposes eight configurable ordered slots instead of inventing labels.

### Transition law

Wheel posture never changes by direct public mutation.

```text
REQUEST NOTCH
    ↓
ETHER BUS // BOARD
    ↓
TRANSIT
    ↓
READY
    ↓
EXPLICIT DISEMBARK
    ↓
ACTIVATE NOTCH
```

## 2. Core = Plumbing

Core owns:

- clocks
- event/signal types
- signal identity
- routing
- transition state
- audit transport
- telemetry transport
- module ownership plumbing
- lifecycle ordering

Core does **not** own:

- opinions
- policy choices
- action choices
- third-party capability behavior
- renderer behavior
- avatar-runtime implementation

## 3. Router Bus

Canonical event families recovered from Spiral OS:

```text
boot
cmd.in
cmd.out
state
exec
error
ui
perf
```

Hakui adds `capability` as a typed route for crystal/module traffic.

### Bus-owned identity

Every live emission receives a fresh monotonically increasing signal ID from `RouterBus`.

Individual crystals/programs do not mint authoritative event IDs.

### Listener safety

Dispatch snapshots the listener set before invoking callbacks so a subscriber may unsubscribe during event handling without invalidating the active iteration.

## 4. Route Table

Recovered routing law:

- predicates inspect `const Signal&`
- routing predicates are intended to be side-effect free
- rules are ordered
- first match wins
- later rules are not consulted once a match is found
- route misses are observable errors
- Core does not invent a semantic fallback

```text
SIGNAL
  ↓
RULE 0 matches? ─ yes → DESTINATION
  ↓ no
RULE 1 matches? ─ yes → DESTINATION
  ↓ no
...
  ↓
ROUTE MISS → ERROR SIGNAL
```

Fallback behavior, if a product wants one later, must exist as an explicit capability/rule rather than hidden Core behavior.

## 5. Monolith Ledger

Recovered Spiral OS behavior: Monolith passively records all Bus events in a bounded ring.

Current invariant:

- default capacity = 1000 events
- oldest event is evicted when full
- `snapshot()` returns current ledger
- `tail(n)` returns newest `n`
- Monolith observes; it does not route or decide

```text
ROUTER BUS ───────────→ LIVE SUBSCRIBERS
     │
     └───────────────→ MONOLITH RING LEDGER
```

## 6. Ether Bus

Ether Bus is a felt transition, not a zero-time function call.

Invariants:

- transition has a minimum ~2–3 second transit window
- no skip path
- explicit Disembark is required
- one passenger/signal at a time in the current proof

Implementation baseline:

- hard floor: 2000 ms
- default transit: 2500 ms
- state: Idle -> Boarding -> Transit -> ReadyToDisembark -> Idle

No `skip()` API exists.

Gameplay input does **not** automatically inherit Ether latency. Ordinary movement remains immediate; Ether Bus is used only for transitions that intentionally require a transit gate.

## 7. Steam Engine

Steam is the execution-energy metaphor and telemetry model.

Recovered laws:

- pistons represent modes/processes
- firing order matters
- engine can idle
- engine can redline
- heat accumulates
- maintenance is explicit
- safety valves contain excess pressure
- pressure should be converted into useful output safely
- prior architecture described a 16-cylinder bus

Current source therefore models:

- 16 deterministic pistons
- pressure
- temperature/heat
- output
- redline
- safety valve
- condenser/cooling
- maintenance request
- explicit steam drain for fault isolation

The steam engine never selects policy or action.

## 8. Pressure Rail

Pressure Rail carries **steam + signal** without confusing them.

```text
STEAM ENGINE ── pressure ──┐
                           ├── PRESSURE RAIL -> ROUTER BUS
SIGNAL PACKET ─────────────┘
```

Energy is owned by `SteamEngine`.
Delivery and event identity are owned by `RouterBus`.
`PressureRail` joins telemetry to the transport packet.

## 9. AUM Field

Canonical Hakui AUM field:

- 7 × 7 node grid
- center = node 3:3
- A/U/M cycle = 9 seconds total
- 3 seconds per phase

```text
A = emergence / expansion
U = flow / sustain
M = compression / return
```

AUM is a governor/lifecycle field. It does not replace Router Bus transport or wheel selection.

## 10. Crystal Lifecycle

A crystal is an optional capability node.

Lifecycle:

```text
DORMANT
  ↓ request emerge
A : EMERGE
  ↓
U : SUSTAIN
  ↓ request return
M : RETURN
  ↓
DORMANT
```

Failure containment law:

> A failed crystal must not become a failed Hakui core.

The Crystal Grid emits a typed error signal and isolates that crystal.

## 11. Crystal Grid vs Crystal Host

Ownership and lifecycle are deliberately split.

### Crystal Grid

Owns:

- AUM-field placement
- lifecycle requests
- Router Bus targeting
- references to live crystals

Does not own module memory.

### Crystal Host

Owns:

- `CrystalModule` memory
- backend lifetime
- safe mount/unmount order

```text
CRYSTAL HOST // owns memory
      ↓
CRYSTAL MODULE
      ↓ exposes
CRYSTAL GRID // owns relationship
      ↓
AUM FIELD + ROUTER BUS
```

Unmount law:

> Grid references are removed before module/backend memory is destroyed.

Mass-unmount occurs in reverse mount order so outer/newer capabilities return before foundational/older modules.

The Host is declared after the Grid in `SpiralKernel`, so C++ reverse destruction order destroys the Host first and allows it to detach every owned module while the Grid still exists.

## 12. Hakui Avatar Rig Law

Hakui owns its avatar schema.

`HakuiSkeleton` contains only first-party rig data:

- bone name
- parent index
- attachment-slot metadata

It contains no:

- SDL types
- Cal3D types
- Boost types
- renderer types
- physics types

Current humanoid hierarchy:

```text
Root
└── Pelvis
    ├── Spine.01
    │   └── Spine.02
    │       └── Chest
    │           ├── Neck → Head
    │           ├── Clavicle.L → UpperArm.L → LowerArm.L → Hand.L
    │           └── Clavicle.R → UpperArm.R → LowerArm.R → Hand.R
    ├── Thigh.L → Shin.L → Foot.L → Toe.L
    └── Thigh.R → Shin.R → Foot.R → Toe.R
```

Runtime libraries translate this schema; they do not define Hakui's canonical skeleton.

## 13. IMVU / Cal3D Crystal Rule

IMVU-Cal3D is **not Hakui's skeleton core**.

It is an optional capability:

```text
HakuiSkeleton
     ↓
ImvuCal3DBackend
     ↓
ImvuCal3DModule
     ↓ exposes
crystal.imvu_skeleton
     ↓ mounted by
CrystalHost
     ↓ lifecycle through
CrystalGrid / AUM
```

`ImvuSkeletonCrystal.hpp`, `ImvuCal3DBackend.hpp`, and `ImvuCal3DModule.hpp` expose zero Cal3D types. Legacy objects live only in `ImvuCal3DBackend.cpp`.

### Module lifetime law

`ImvuCal3DModule` owns both backend and crystal.

Member order is deliberate:

```text
backend_  // constructed first, destroyed last
crystal_  // constructed second, destroyed first
```

The crystal's hooks therefore cannot outlive the backend object they target.

### Minimal Cal3D slice

Hakui does not build the full IMVU-Cal3D library at this stage.

The optional target currently includes only the skeletal transform slice:

```text
bone.cpp
bonetransform.cpp
corebone.cpp
coreskeleton.cpp
error.cpp
matrix.cpp
memory.cpp
platform.cpp
quaternion.cpp
skeleton.cpp
vector.cpp
```

Explicitly excluded from the backend:

- loader/XML parsing
- RapidXML
- saver pipeline
- model/renderer pipeline
- unrelated mesh-loading infrastructure

IMVU's `USE_CAL3D_WITH_CPP_11=1` path is enabled so this slice uses standard-library smart pointers/optional instead of requiring Boost.

### Dependency law

```text
spiral_core               = plain C++20
hakui_avatar_rig          = plain C++20
hakui client              = spiral_core + hakui_avatar_rig + SDL3
spiral_imvu_cal3d_backend = OPTIONAL translator/module capability
hakui_cal3d_skeleton      = OPTIONAL LGPL legacy runtime slice
```

The Hakui executable never links Cal3D directly.

## 14. Live Hakui Client Bridge

`HakuiApp` now owns a `SpiralKernel` as part of the native client heartbeat.

Current integration:

```text
Hakui boot
  ↓
Router Bus boot signal
  ↓
Monolith audit

frame dt
  ↓
SpiralKernel.tick()
  ├── Steam state
  ├── Ether clock
  ├── AUM phase
  └── Crystal lifecycle

locomotion change
  ↓
immediate gameplay state change
  ↓
Router Bus state signal
```

SDL is an outer bridge only. Spiral Core contains no SDL dependency.

The debug window title may expose AUM phase + Monolith event count as observable telemetry.

On client shutdown, `CrystalHost.unmountAll()` is called before renderer/platform teardown.

## 15. Dependency Firewall

`cmake/DependencyFirewall.cmake` enforces first-party source boundaries during configuration.

Forbidden direct includes in `spiral_core` and `hakui_avatar_rig`:

```text
SDL3/
cal3d/
boost/
rapidxml
```

The explicit optional `src/spiral/crystal/backend/` directory is excluded from the Spiral firewall because that directory is the intentional adapter boundary.

This means the dependency law is executable configuration policy, not only documentation.

## 16. Current Source Layout

```text
src/
├── avatar/
│   ├── HakuiSkeleton.hpp/.cpp
│   └── AvatarAttachment.hpp
└── spiral/
    ├── SpiralKernel.hpp/.cpp
    ├── core/
    │   ├── SpiralTypes.hpp
    │   └── StopGate.hpp
    ├── bus/
    │   ├── RouterBus.hpp/.cpp
    │   └── EtherBus.hpp/.cpp
    ├── routing/
    │   └── RouteTable.hpp
    ├── ledger/
    │   └── MonolithLedger.hpp/.cpp
    ├── wheel/
    │   └── OctopusWheel.hpp
    ├── engine/
    │   ├── SteamEngine.hpp/.cpp
    │   └── PressureRail.hpp
    ├── aum/
    │   └── AUMField.hpp/.cpp
    └── crystal/
        ├── Crystal.hpp
        ├── CrystalModule.hpp
        ├── CrystalGrid.hpp/.cpp
        ├── CrystalHost.hpp/.cpp
        ├── ImvuSkeletonCrystal.hpp/.cpp
        └── backend/
            ├── ImvuCal3DBackend.hpp/.cpp
            └── ImvuCal3DModule.hpp/.cpp
```

## 17. Build Gates and Targets

Core targets:

```text
spiral_core
hakui_avatar_rig
```

Native client:

```text
HAKUI_BUILD_NATIVE_CLIENT=ON
→ SDL3
→ hakui
```

Optional legacy capability:

```text
HAKUI_ENABLE_IMVU_CAL3D=ON
→ hakui_cal3d_skeleton
→ spiral_imvu_cal3d_backend
```

Optional invariant specs:

```text
HAKUI_ENABLE_SPIRAL_LOGIC_SPECS=ON
→ spiral_logic_spec

HAKUI_ENABLE_IMVU_BACKEND_SPECS=ON
→ imvu_cal3d_backend_spec
```

Cal3D is OFF by default.
Both spec executables are OFF by default.

## 18. Validation Law

Do not compile-chase architecture.

Order:

1. freeze invariants
2. code plumbing
3. write source-level contracts
4. separate ownership graph
5. separate dependency graph
6. validate `spiral_core` with native client OFF and Cal3D OFF
7. validate Hakui avatar rig independently
8. validate native client
9. validate optional IMVU translator/runtime separately
10. only then connect live GPU-skinned avatar rendering/animation

Automatic CI remains disabled.

The manual core workflow is configured to use:

```text
HAKUI_BUILD_NATIVE_CLIENT=OFF
HAKUI_ENABLE_IMVU_CAL3D=OFF
HAKUI_ENABLE_SPIRAL_LOGIC_SPECS=ON
```

and to build only the `spiral_logic_spec` target.

No source-level contract in this branch should be described as passing until it has actually been run.
