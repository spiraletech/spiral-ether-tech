# INOKUI — Native Graphics Overhaul Roadmap

**Canonical runtime:** HAKUI C++20 / SDL3 GPU in `spiraletech/spiral-ether-tech`. HOME owns world truth, HAKUI owns body movement, INOKUI owns presentation, ENTROKOI owns camera/observer rules. Do not replace the engine with HTML.

## Existing visual limitations
- `DebugWorldRenderer.cpp` renders non-indexed colored cube meshes.
- Shader bytecode currently comes from SDL's temporary GPU cube test examples.
- Avatar/environment/vehicles are mostly cube assemblies; the native GPU skinned human mesh pipeline is not connected.
- M4A/M4B GLB importing/derivation work exists separately in draft PR #11; that is not yet GPU-rendered gameplay.

## G1 — Native style and compatibility renderer (this branch)
- Renderer-independent material descriptors (base color, roughness, metallic, emissive intent), explicit lighting and quality settings.
- Directional + hemisphere light mapped into compatibility cube vertex colors; deliberate background palette.
- CTest verifies finite/bounded color values, material contrast and quality configuration.
- This pass does **not** implement physically based shader lighting, actual shadows, MSAA, bloom, indexed scene meshes or skinned render.

## G2 — First-party native mesh and shader backend
- Replace borrowed SDL test shaders with reproducibly built first-party vertex/fragment shaders for required GPU backends.
- Implement indexed meshes (positions, normals, UVs, joints, weights), separate mesh storage and shader layout reflection/validation.
- Integrate existing GLB importer to draw a neutral skinned base in bind pose; keep debug cubes as fallback.

## G3 — Animation, skinning and garments
- Bone palette/animation sampling and facial morphs; stable normal transforms and deforming real meshes.
- Body morph retargeting for proportional changes; separately skinned hair, clothing and accessories.
- Test idle/walk/run/skate and first-person camera masking on Windows GPU.

## G4 — World visual quality
- Author full scene assets (rooms, city, cars, furniture, floors) with material/mesh distinction, not collision primitives used as final art.
- Dynamic lighting, shadows, alpha surfaces, decals, atmosphere/fog, calibrated exposure, AA and optional bloom.
- Profile tier controls only advertise options actually implemented and GPU-tested.

## G5 — Native reliability/performance and release
- Device-loss/recreation, frustum culling, resource lifetimes, GPU profiling, deterministic render data and rendering-budget tests.
- Windows/driver hardware matrix; verified frame captures, stable FPS, no regressions to gameplay.
- Only promote each stage after passing CI plus real visual QA. Draft branches are not releases.

**HTML laboratory policy:** art and material previews are permitted for review but must be labeled as non-authoritative and not evidence of SDL3 graphics runtime success.
