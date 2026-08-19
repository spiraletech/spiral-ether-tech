# PROJECT HAKUI — Native Client v0.2

Hakui is a native C++20 client with SDL3 GPU and an IMVU-descended skeletal animation substrate.

## Stack

- C++20 — Hakui systems/runtime
- SDL3 — platform, window, input, audio, GPU abstraction
- SDL3 GPU — custom renderer
- IMVU Cal3D fork — optional skeletal animation runtime
- CMake — build orchestration
- Jolt Physics — planned

## Avatar runtime

Hakui uses IMVU's public Cal3D fork behind `HakuiSkeleton`. The runtime gives us skeletal animation, animation blending, weighted skinning, runtime mesh attachment/detachment, morphs, LOD, and a path toward hair/clothing systems.

Hakui defines its own humanoid hierarchy and stable attachment slots for body, head, hair, face, torso, back, waist, hands, feet, skateboard, BMX, and vehicle seating.

## Build

```bash
cmake -S . -B build
cmake --build build --config Release
```

Disable Cal3D for renderer-only development:

```bash
cmake -S . -B build -DHAKUI_ENABLE_IMVU_CAL3D=OFF
```

See `THIRD_PARTY.md` for license notes.
