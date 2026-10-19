# HAKUI — Derivable Mannequin Asset Contract (M4B)

## One neutral base, unlimited authored profiles

HAKUI's avatar foundation is a professionally modeled **neutral human** with a deformable mesh, skeleton, shape targets, reusable skinning, detachable wardrobe and stable attachment points. Character identities are *data*, not different engines or hard-coded bodies. This system is **not** a procedural cylinder/person generator.

Suggested source: `assets/avatars/base_human/base_human.glb`, plus the editable Blender project.

### Required Blender/glTF attributes

* Binary glTF 2.0, documented Y-up/meters/forward orientation, indexed triangles and realistic humanoid surface topology (shoulder/hip/elbow/knee edge loops).
* Neutral A-pose, stable skinned rig, inverse bind transforms, `JOINTS_0`/`WEIGHTS_0`, with up to four normalized influences per vertex.
* Required joint names: `Pelvis`, `Waist`, `Chest`, `Neck`, `Head`, `Hand_L`, `Hand_R`, `Thigh_L`, `Thigh_R`, `Calf_L`, `Calf_R`, `Foot_L`, `Foot_R`. Finger, face, spine and toe bones are encouraged. Exported bone order and hierarchy must be consistent.
* Separate material names: `Mat_Skin`, `Mat_Eyes` and optional `Mat_Hair`, `Mat_Shirt_Outer`, `Mat_Shirt_Under`, `Mat_Denim`, `Mat_Shoes`, `Mat_Accessories`.
* Each customizable shape is a **real authored vertex delta**, with separate positive and negative morphs named `SHAPE_<CHANNEL>_POS` and `SHAPE_<CHANNEL>_NEG`, aligned to the base-mesh vertices. Channels: `STATURE`, `SHOULDER_WIDTH`, `CHEST_VOLUME`, `WAIST_WIDTH`, `HIP_WIDTH`, `ARM_LENGTH`, `LEG_LENGTH`, `HEAD_SIZE`, `JAW_WIDTH`, `NOSE_SHAPE`, `EYE_SHAPE`, `MOUTH_SHAPE`.
* Missing shape targets may remain absent for channels that must stay neutral; nonzero values on absent channels **must error**, not silently create fake anatomy.

### Garments / accessories

Separate source GLB per hair, shirt, pants, shoes and accessory, compatible with the neutral rig (canonical bone names and parents). Available slots: `HAIR`, `HEADWEAR`, `SHIRT_OUTER`, `SHIRT_UNDER`, `PANTS`, `FOOTWEAR`, `ACCESSORY_NECK`, `HAND_LEFT`, `HAND_RIGHT`, `BOARD_BACK`.

### Current native implementation state

**M4A** parses supported GLB mesh, indices, material names, joint data, skinning attributes, morphs and animation channels. **M4B** derives non-destructive mesh-shape variants from an authored base, stores material and attachment selection, validates canonical attachment anchors and bone-name compatibility. Both run as first-party C++20 modules, independent of SDL/Cal3D.

**Not yet implemented:** rendering/animation in SDL GPU, skeletal retargeting of length-changing body morphs, garment fitting, load/merge of separate garment meshes, high-quality neutral human geometry generation. In particular, morph-only shape edits do not update inverse bind or joint positions; body-length morphs cannot be considered visually accepted without later bone-aware retargeting.

### Delivery from Gemini / art pipeline

1. `base_human.blend` plus `base_human.glb` (real mesh, not a proxy).
2. Neutral front/side/back screenshots and close-ups of face, hands and shoulder topology.
3. Screenshot and exported-mesh evidence of at least one functioning shoulder-width and jaw-width `_POS` and `_NEG` target.
4. One rig-compatible shirt GLB plus optional hair GLB.
5. Skin/animation deformation screenshots: arms raised, knee bend, walk and seated.
6. Actual Blender export/reimport validation report, tested Blender version, triangle/vertex/bone counts, and clear unresolved failures.

Do not replace the native C++ engine with HTML or report completion based on source-code presence alone.
