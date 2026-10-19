#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace hakui::import {

struct SkinnedVertex {
    std::array<float, 3> position{};
    std::array<float, 3> normal{0.f, 1.f, 0.f};
    std::array<float, 2> uv{};
    std::array<std::uint16_t, 4> joints{};
    std::array<float, 4> weights{1.f, 0.f, 0.f, 0.f};
};

struct Submesh {
    std::string name;
    std::string material;
    std::uint32_t firstIndex = 0;
    std::uint32_t indexCount = 0;
};

struct Joint {
    std::string name;
    int parent = -1; // index into joints; -1 = root
    std::array<float, 3> translation{};
    std::array<float, 4> rotation{0.f, 0.f, 0.f, 1.f};
    std::array<float, 3> scale{1.f, 1.f, 1.f};
    std::array<float, 16> inverseBind{}; // column-major, glTF convention
};

struct MorphTarget {
    std::string name;
    std::vector<std::array<float, 3>> positionDeltas;
};

struct AnimationChannel {
    std::string clip;
    std::uint32_t joint = 0;
    std::string path; // translation / rotation / scale
    std::string interpolation; // LINEAR / STEP
    std::vector<float> times;
    std::vector<std::array<float, 4>> values; // xyz for translation/scale, xyzw rotation
};

struct GlbMesh {
    std::vector<SkinnedVertex> vertices;
    std::vector<std::uint32_t> indices;
    std::vector<Submesh> submeshes;
    std::vector<Joint> joints;
    std::vector<MorphTarget> morphTargets;
    std::vector<AnimationChannel> animationChannels;
};

struct LoadResult {
    bool ok = false;
    std::string error;
    GlbMesh mesh;
};

// Strict, bounded glTF 2.0 binary loader. No renderer/SDL/Cal3D dependencies.
// Supports one active skin and skinned primitives, with explicit errors for
// unsupported vertex/animation formats. Never substitutes debug geometry.
LoadResult loadGlb(const std::filesystem::path& path);

} // namespace hakui::import
