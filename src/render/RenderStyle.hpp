#pragma once

#include <array>
#include <cstdint>
#include "world/WorldGeometry.hpp"

namespace hakui::graphics {
struct Color3 { float r = 0.0f; float g = 0.0f; float b = 0.0f; };
struct SurfaceStyle {
    Color3 albedo{};
    float roughness = 1.0f;
    float metallic = 0.0f;
    float emission = 0.0f;
};

// Native renderer contracts; G1 compatibility path bakes lighting into
// vertex colors. These are NOT dynamic PBR or shadow implementations.
enum class QualityTier : std::uint8_t { Low, Balanced, High };
struct QualitySettings {
    QualityTier tier = QualityTier::Balanced;
    std::uint32_t maxVisibleDraws = 4096;
    float renderScale = 1.0f;
    std::uint32_t requestedMsaa = 1;
    bool enablePostFx = false;
};
struct LightingSettings {
    Color3 keyDirection{0.35f, 0.82f, -0.45f};
    float ambient = 0.42f;
    float keyIntensity = 0.68f;
    float fillIntensity = 0.12f;
    float exposure = 1.05f;
};
QualitySettings qualityPreset(QualityTier tier) noexcept;
bool validQualitySettings(const QualitySettings& settings) noexcept;
SurfaceStyle materialStyle(MaterialRole material) noexcept;
Color3 shadeDebugSurface(MaterialRole material, Color3 outwardNormal,
                         const LightingSettings& lighting = {}) noexcept;
Color3 clearColor(bool paused) noexcept;
} // namespace hakui::graphics
