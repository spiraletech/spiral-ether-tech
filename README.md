# PROJECT HAKUI — Native Client v0.3

Hakui is a native C++20 social/action world client with SDL3 GPU and an IMVU-descended skeletal animation substrate.

## v0.3 milestone

Hakui now has its first visible 3D world slice:

- native SDL3 GPU graphics pipeline
- depth-tested 3D rendering
- high-angle follow camera
- persistent player world transform
- WASD on-foot movement
- Shift sprint
- grey cuboid humanoid debug proxy
- large world floor for scale/movement testing
- live position display in the window title

The cuboid avatar is intentionally a debug proxy. Its purpose is to prove camera, scale, input, movement, draw calls, and humanoid proportions before the Cal3D bone palette is connected to GPU skinning.

## Stack

- **C++20** — Hakui systems/runtime
- **SDL3 3.4.14** — platform, window, input, audio, GPU abstraction
- **SDL3 GPU** — custom 3D renderer
- **IMVU Cal3D fork** — optional skeletal animation runtime
- **CMake** — build orchestration
- **GitHub Actions** — native Windows compile validation
- **Jolt Physics** — planned

## Avatar runtime

Hakui uses IMVU's public Cal3D fork behind `HakuiSkeleton`. The runtime gives us skeletal animation, animation blending, weighted skinning, runtime mesh attachment/detachment, morphs, LOD, and a path toward hair/clothing systems.

Hakui defines its own humanoid hierarchy and stable attachment slots for body, head, hair, face, torso, back, waist, hands, feet, skateboard, BMX, and vehicle seating.

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

Only on-foot movement is simulated in v0.3. The other modes already exist in the locomotion router and are the next movement-controller targets.

## Build

```bash
cmake -S . -B build
cmake --build build --config Release
```

Disable Cal3D for renderer-only development:

```bash
cmake -S . -B build -DHAKUI_ENABLE_IMVU_CAL3D=OFF
```

## Current render pipeline

```text
PlayerState
   ↓
WASD locomotion
   ↓
High-angle follow camera
   ↓
Hakui matrices
   ↓
SDL3 GPU command buffer
   ↓
Depth-tested render pass
   ↓
World floor + humanoid debug proxy
```

The next renderer milestone is to replace the block proxy with a skinned mesh driven by `HakuiSkeleton` / IMVU-Cal3D bone transforms.

See `THIRD_PARTY.md` for license notes.
