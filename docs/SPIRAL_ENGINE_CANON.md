# SPIRAL ENGINE CANON — Hakui Integration

Status: **logic-first / not wired into CMake yet**

This document freezes the recovered Spiral OS machinery before Hakui integrates third-party runtime code.

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

## 2. Core = Plumbing

Core owns:

- clocks
- event/signal types
- routing
- transition state
- telemetry transport
- lifecycle ordering

Core does **not** own:

- opinions
- policy choices
- action choices
- third-party capability behavior

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

The Router Bus is deliberately live-only at this layer. No analytics, streaks, or persistence are required for the core transport.

## 4. Ether Bus

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

## 5. Steam Engine

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

## 6. Pressure Rail

Pressure Rail carries **steam + signal** without confusing them.

```text
STEAM ENGINE ── pressure ──┐
                           ├── PRESSURE RAIL -> ROUTER BUS
SIGNAL PACKET ─────────────┘
```

Energy is owned by `SteamEngine`.
Delivery is owned by `RouterBus`.
`PressureRail` only joins the telemetry at transport time.

## 7. AUM Field

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

## 8. Crystal Logic

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

## 9. IMVU / Cal3D Rule

IMVU-Cal3D is **not Hakui's skeleton core**.

It is:

```text
crystal.imvu_skeleton
```

The public IMVU Cal3D runtime may later be connected through backend hooks from a separate adapter translation unit.

`ImvuSkeletonCrystal.hpp` intentionally contains **zero Cal3D headers**.

Therefore old Boost/RapidXML requirements cannot infect the Spiral/Hakui core dependency graph.

## 10. Current Source Layout

```text
src/spiral/
├── SpiralKernel.hpp/.cpp
├── core/
│   ├── SpiralTypes.hpp
│   └── StopGate.hpp
├── bus/
│   ├── RouterBus.hpp/.cpp
│   └── EtherBus.hpp/.cpp
├── wheel/
│   └── OctopusWheel.hpp
├── engine/
│   ├── SteamEngine.hpp/.cpp
│   └── PressureRail.hpp
├── aum/
│   └── AUMField.hpp/.cpp
└── crystal/
    ├── Crystal.hpp
    ├── CrystalGrid.hpp/.cpp
    └── ImvuSkeletonCrystal.hpp/.cpp
```

## 11. Integration Order

Do not compile-chase the architecture.

Order:

1. freeze invariants
2. code Router Bus
3. code Ether Bus
4. code orthogonal Octopus wheels
5. code Steam Engine / Pressure Rail
6. code AUM field
7. code Crystal Grid
8. isolate IMVU-Cal3D as a crystal
9. write logic-level tests
10. only then wire modules into CMake
11. compile core without Cal3D
12. compile optional Cal3D adapter separately

Automatic CI remains disabled during steps 1–9.
