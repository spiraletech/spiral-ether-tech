# Third-Party Components

## SDL3

Hakui uses SDL3 as its native platform, input/audio, and GPU abstraction layer.

- Project: SDL3
- Repository: `libsdl-org/SDL`
- Version: `release-3.4.14`
- License: zlib
- Integration: CMake `FetchContent`

### Debug shader bootstrap

The current debug renderer temporarily consumes SDL's precompiled cube shader headers from the fetched SDL source tree, providing DXIL, MSL, and SPIR-V bootstrap shaders.

The world layout, player controls, camera, matrices, humanoid debug geometry, and renderer architecture around those shader blobs are Hakui code. The bootstrap shaders should eventually be replaced by Hakui-owned shader assets.

## IMVU Cal3D — optional crystal backend

Hakui optionally integrates the public IMVU fork of Cal3D as a **legacy skeletal transform backend**, not as Hakui's canonical avatar definition.

- Project: IMVU Cal3D
- Repository: `imvu/cal3d`
- Pinned commit: `8522b45cdc26960f779323cc9a0ef50678eda7de`
- License: GNU Lesser General Public License v2.1 or later
- Default: **disabled**
- Legacy runtime target: `hakui_cal3d_skeleton`
- Hakui adapter target: `spiral_imvu_cal3d_backend`

### Isolation boundary

Hakui's first-party `HakuiSkeleton` owns the humanoid hierarchy and attachment-slot conventions. It contains no Cal3D types.

The optional translation path is:

```text
HakuiSkeleton
      ↓
ImvuCal3DBackend
      ↓
ImvuCal3DModule
      ↓
crystal.imvu_skeleton
```

`ImvuCal3DBackend.hpp`, `ImvuCal3DModule.hpp`, and `ImvuSkeletonCrystal.hpp` expose no Cal3D types. Cal3D objects are confined to the backend implementation translation unit.

### Selected source slice

Hakui does not currently build the full Cal3D project. The optional target selects the skeletal-transform subset needed for the proof:

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

Loader/XML/saver/model/renderer sources are excluded. This keeps RapidXML out of the selected backend.

The IMVU fork's `USE_CAL3D_WITH_CPP_11=1` path is enabled for this target so its standard-library smart-pointer/optional path is used instead of the legacy Boost path.

### Licensing note

The optional Cal3D runtime is built as a separate shared library target. Distribution must preserve applicable LGPL notices and requirements for that component. Hakui's first-party code remains architecturally separated from the optional Cal3D runtime.

The project does not bundle or claim IMVU's proprietary production avatar rig or avatar assets.
