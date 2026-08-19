# Third-Party Components

## SDL3

Hakui uses SDL3 as its native platform and GPU abstraction layer.

- Project: SDL3
- Repository: `libsdl-org/SDL`
- Version: `release-3.4.14`
- License: zlib
- Integration: CMake `FetchContent`

### v0.3 debug shader bootstrap

Hakui v0.3 temporarily compiles against SDL's precompiled `test/testgpu/cube.*` shader headers from the fetched SDL source tree. They provide DXIL, MSL, and SPIR-V versions of the tiny vertex/fragment shader used by the first debug 3D renderer.

The geometry, world layout, player controls, camera, matrices, humanoid proportions, and renderer architecture around those shader blobs are Hakui code. The SDL test shaders are only a bootstrap dependency and will be replaced by Hakui-owned shader assets as the renderer develops.

## IMVU Cal3D

Hakui optionally integrates IMVU's public fork of Cal3D as its skeletal animation substrate.

- Project: IMVU Cal3D
- Repository: `imvu/cal3d`
- Pinned commit: `8522b45cdc26960f779323cc9a0ef50678eda7de`
- License: GNU Lesser General Public License v2.1 or later
- Integration: dynamically built as `hakui_cal3d`

Hakui's `HakuiSkeleton` hierarchy and attachment-slot conventions are original Hakui code. The project does not bundle or claim IMVU's proprietary production avatar skeleton or avatar assets.
