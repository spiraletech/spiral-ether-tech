#include "render/RenderStyle.hpp"

#include <algorithm>
#include <cmath>

namespace hakui::graphics {
namespace {
float clamp01(float x) noexcept {
    return std::isfinite(x) ? std::clamp(x, 0.0f, 1.0f) : 0.0f;
}
Color3 normalize(Color3 v) noexcept {
    if (!std::isfinite(v.r) || !std::isfinite(v.g) || !std::isfinite(v.b)) return {0.f, 1.f, 0.f};
    const float length = std::sqrt(v.r*v.r + v.g*v.g + v.b*v.b);
    if (length < 1.0e-6f) return {0.f, 1.f, 0.f};
    return {v.r/length, v.g/length, v.b/length};
}
float dot(Color3 a, Color3 b) noexcept { return a.r*b.r + a.g*b.g + a.b*b.b; }
float toneChannel(float value, float exposure) noexcept {
    // Soft-knee display approximation for the existing vertex-color renderer.
    // A future HDR shader must do scene-linear tone mapping instead.
    const float x = std::max(0.0f, value * exposure);
    const float mapped = x / (x + 0.55f);
    return clamp01(std::pow(mapped, 1.0f/2.2f));
}
}

QualitySettings qualityPreset(QualityTier tier) noexcept {
    switch (tier) {
        case QualityTier::Low: return {tier, 1400, 0.75f, 1, false};
        case QualityTier::Balanced: return {tier, 4096, 1.00f, 1, false};
        case QualityTier::High: return {tier, 8192, 1.00f, 4, true};
    }
    return {QualityTier::Balanced, 4096, 1.00f, 1, false};
}

bool validQualitySettings(const QualitySettings& settings) noexcept {
    const auto tier = static_cast<std::uint8_t>(settings.tier);
    return tier <= static_cast<std::uint8_t>(QualityTier::High) &&
           settings.maxVisibleDraws >= 128 && settings.maxVisibleDraws <= 65536 &&
           std::isfinite(settings.renderScale) && settings.renderScale >= 0.5f &&
           settings.renderScale <= 1.0f &&
           (settings.requestedMsaa == 1 || settings.requestedMsaa == 2 ||
            settings.requestedMsaa == 4 || settings.requestedMsaa == 8);
}

SurfaceStyle materialStyle(MaterialRole material) noexcept {
    switch (material) {
        case MaterialRole::PowderConcrete: return {{0.57f, 0.61f, 0.67f}, 0.95f, 0.f, 0.f};
        case MaterialRole::IndustrialDark: return {{0.075f, 0.087f, 0.12f}, 0.78f, 0.48f, 0.f};
        case MaterialRole::CrtCyan: return {{0.035f, 0.54f, 0.64f}, 0.32f, 0.1f, 0.38f};
        case MaterialRole::SignalMagenta: return {{0.60f, 0.055f, 0.36f}, 0.42f, 0.1f, 0.34f};
        case MaterialRole::SodiumAmber: return {{0.72f, 0.34f, 0.055f}, 0.62f, 0.16f, 0.26f};
        case MaterialRole::VoidBlack: return {{0.011f, 0.013f, 0.020f}, 0.98f, 0.f, 0.f};
        case MaterialRole::TerminalGreen: return {{0.075f, 0.63f, 0.33f}, 0.44f, 0.08f, 0.28f};
        case MaterialRole::HazardRed: return {{0.72f, 0.045f, 0.07f}, 0.55f, 0.1f, 0.24f};
    }
    return {{0.57f, 0.61f, 0.67f}, 0.95f, 0.f, 0.f};
}

Color3 shadeDebugSurface(MaterialRole material, Color3 outwardNormal,
                         const LightingSettings& lighting) noexcept {
    const SurfaceStyle surface = materialStyle(material);
    const Color3 normal = normalize(outwardNormal);
    const Color3 key = normalize(lighting.keyDirection);
    const float lambert = std::max(0.0f, dot(normal, key));
    const float hemi = 0.5f + 0.5f * normal.g;
    const float ambient = clamp01(lighting.ambient);
    const float keyPower = clamp01(lighting.keyIntensity);
    const float fill = clamp01(lighting.fillIntensity);
    const float luminance = ambient * (0.65f + 0.35f*hemi) + keyPower*lambert + fill;
    const float emission = clamp01(surface.emission);
    const float exposure = std::isfinite(lighting.exposure)
        ? std::clamp(lighting.exposure, 0.25f, 4.0f) : 1.0f;
    return {
        toneChannel(surface.albedo.r*luminance + surface.albedo.r*emission, exposure),
        toneChannel(surface.albedo.g*luminance + surface.albedo.g*emission, exposure),
        toneChannel(surface.albedo.b*luminance + surface.albedo.b*emission, exposure)
    };
}

Color3 clearColor(bool paused) noexcept {
    return paused ? Color3{0.009f, 0.010f, 0.015f}
                  : Color3{0.006f, 0.009f, 0.019f};
}

} // namespace hakui::graphics
