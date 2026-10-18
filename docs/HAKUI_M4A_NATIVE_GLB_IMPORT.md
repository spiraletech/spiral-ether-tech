# M4A — Native skinned GLB import (experimental)

**Authority:** `spiral-ether-tech` C++20 runtime. This branch is not a migration of the Gemini HTML prototype.

## Implemented

`hakui::import::loadGlb(path)` parses an embedded-buffer glTF 2.0 .glb into native, renderer-independent data, reporting an explicit error and an empty asset on failure. Supported import data:

- skinned TRIANGLES primitives with POSITION (float3), JOINTS_0 (u8/u16), WEIGHTS_0 (float or normalized u8/u16);
- optional NORMAL (float3), TEXCOORD_0 (float2/u8/u16), u8/u16/u32 indices, multiple mesh nodes and primitives sharing one skin;
- material names and submesh index ranges;
- skin joint names, nearest joint ancestors, node-local TRS, inverse-bind matrices;
- vertex-position morph targets and named shape keys;
- LINEAR / STEP translation, rotation and scale animation channels;
- bounded GLB chunks, bufferViews, accessor strides and offsets, index/joint validation and weight normalization.

The importer is **not** a GPU renderer and is deliberately independent of SDL, Cal3D, HOME and the gameplay layer.

## Verification

The fixture in `tests/hakui/GlbImportSpec.cpp` generates a real skinned GLB with binary POSITION/NORMAL/UV/JOINTS/WEIGHTS/index/inverse bind/morph/animation accessors. Its test verifies actual decoded values and rejects out-of-bounds indices, missing files and corrupted headers.

In a full native checkout:

```sh
cmake -S . -B build-m4a -DHAKUI_BUILD_NATIVE_CLIENT=OFF -DBUILD_TESTING=ON
cmake --build build-m4a --config Debug
ctest --test-dir build-m4a -C Debug --output-on-failure
```

Local standalone module: GCC 14.2 CMake/CTest and ASan+UBSan passed. **A complete Windows native build, end-to-end in-game rendering, Blender export of Ether, and GitHub CI have not been validated by that test.**

## Known constraints / next gates

- Single skin; one embedded .glb BIN buffer; no external URIs.
- Mesh nodes must have no explicit local transform. Joint `matrix` transforms require TRS decomposition and are rejected; joint TRS is supported.
- No sparse accessors, CUBICSPLINE tracks, animation weight channels or GLB texture decoding yet.
- The importer stores inverse-bind matrices, named morph deltas and animation samples but does not apply them to GPU vertices.
- Node hierarchy/coordinate transform verification and shader-side skinning are M4B acceptance criteria; do not label M4A as a rendered, animated character.
- No fallback mannequin is invented or incorrectly returned as a successfully loaded GLB.

**Do not merge without Windows CI, validation against Blender-exported `ether_human_001.glb`, and reviewer sign-off.**
