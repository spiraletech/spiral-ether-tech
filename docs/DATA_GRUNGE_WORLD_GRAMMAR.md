# HAKUI DATA GRUNGE World Grammar

Status: v0.65 specimen contract, inherited and extended by local v0.7/v0.75
embodiment pass.

The first HAKUI environment is an authored but editable assembly, not a baked
scene and not a realism showcase. Its permanent visual thesis is stark,
modular, surreal, legible, and deliberately incomplete.

## Layer law

```text
WORLD DESCRIPTION
    primitives + material roles + descriptive affordances
        ↓
GAMEPLAY / SIMULATION
    movement + interaction + tabletop + combat + respawn
        ↓
PRESENTATION
    camera roles + full-body pose + HUD + audio feedback
        ↓
SDL RENDERER
    geometry submission only; never authoritative gameplay
```

The renderer does not decide whether a player can stand on a ramp, sit, place a
wager, land an attack, take damage, fall, or respawn.

## Primitive vocabulary

`WorldPrimitive` provides position, dimensions, rotation, material role, and
repeat offsets. Repetition is the first editability primitive: walls, rails,
datum lines, markers, and furniture modules can be extended without adding
renderer code.

Canonical v0.65 roles:

- floors, walls, platforms, and ramps;
- furniture, casino tables, and terminals;
- signage and monuments;
- void markers and sparse architectural masses.

## Material vocabulary

Materials are semantic, not texture filenames:

- `PowderConcrete`: pale plaster/concrete mass and sculptural structure;
- `IndustrialDark`: furniture, datum slabs, and machine-like structure;
- `CrtCyan` and `SignalMagenta`: selective digital edges and navigation cues;
- `SodiumAmber`: sparse industrial warmth and anchor lighting;
- `TerminalGreen`: operable fictional-machine feedback;
- `HazardRed`: boundaries, danger, and combat datum legibility;
- `VoidBlack`: absence, depth, and black-space transitions.

A future high-fidelity renderer may add powder, roughness, fog, decals, and
lighting while preserving these meanings.

## Affordance vocabulary

`WorldAffordanceVolume` advertises possible actions. It contains no physics,
casino, combat, equipment, or AI implementation.

Current/future semantics include:

```text
Rideable      Grindable       Transition      Launch
Landing       ManualZone      StallAnchor     Seat
CasinoAnchor Terminal         FightZone       SparAnchor
SpectatorZone RespawnVolume   Void            DuelZone
ArcheryLane   WeaponRack      Target
```

Examples:

- `FightZone` says a space supports combat; `CombatSimulation` owns combat.
- `DuelZone` may later support formal sword play but contains no sword rules.
- `ArcheryLane` describes ranged-practice space but contains no ballistics.
- `WeaponRack` advertises interaction; equipment systems will interpret it.

## Third-person camera grammar

HAKUI remains third-person first. The current roles are gameplay follow,
interaction framing, and combat framing. Target, duel, spectator, and director
roles are named extension points for later systems. None assumes a first-person
weapon model.

Every future combat discipline must preserve visible stance, footwork, body
rotation, clothing/equipment, weapon orientation, and cinematic readability.

## Combat grammar

```text
INPUT / AI INTENT
        ↓
COMBAT CONTROLLER
        ↓
DISCIPLINE INTERPRETER
        ↓
ATTACK / DEFENSE RESOLUTION
        ↓
HIT / DAMAGE EVENTS
        ↓
GAMEPLAY STATE
        ↓
ANIMATION / AUDIO / CAMERA / HUD
```

Shared combat owns state, intent, equipment discipline, stamina, targeting,
hit/damage events, interruption, knockdown, and recovery. Unarmed currently
interprets jab and cross. Sword and bow semantics are declared but disabled.

ETHER'S LAB rule:

> Build one combat language. Let fists, swords and bows speak different dialects of it.

## v0.7 embodiment grammar

Semantic state must have a readable body. The avatar, skateboard, BMX, couch,
Fusion table, and sparring datum are presented from gameplay/world state; the
renderer cannot create mounts, hits, seats, or anchors by inference.

- interaction entry and exit use `WorldAnchor` records owned by affordances;
- the FightZone exposes both player and opponent anchors;
- mobility silhouettes near spawn teach the `2`/`3` modes without a generic
  debug menu;
- skateboard/BMX meshes attach only while their deterministic locomotion mode
  is active;
- Car remains labeled and immobile until it has an authored representation;
- combat and interaction framing remain third-person and full-body readable.

## v0.75 expressive movement grammar

World geometry advertises opportunities; it never contains trick code.
`Grindable`, `Transition`, `Launch`, `Landing`, `ManualZone`, and `StallAnchor`
are interpreted by the active discipline. A skateboard maps them to truck/board
grinds, ollies, and board manuals. BMX maps the same vocabulary to peg grinds,
bunny hops, and wheel manuals.

Movement lines should connect at least three readable opportunities. The first
specimen line uses a powder manual strip, low cyan rail, transfer kicker,
gallery ramp, and elevated edge. Luminous accents communicate affordance class;
the surrounding concrete and negative space remain visually restrained.

Shared ride state is deterministic and renderer-independent. Rendering may
pose the board, bicycle, and avatar from phase, trick, balance, steering, and
landing quality, but it may not decide whether a trick succeeds.

## Fidelity rule

Editability and silhouette clarity survive every fidelity upgrade. Photorealism
is optional; impossible architecture, corrupted geometry, living displays,
altered physics, and other DATA GRUNGE anomalies are first-class future targets.
