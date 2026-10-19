#include "render/RenderStyle.hpp"

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace {
void expect(bool value, const char* message) {
    if (!value) { std::cerr << "[FAIL] " << message << '\n'; std::exit(1); }
}
bool in01(float x) { return std::isfinite(x) && x >= 0.f && x <= 1.f; }
}

int main() {
    using namespace hakui;
    using namespace hakui::graphics;
    for (auto tier : {QualityTier::Low, QualityTier::Balanced, QualityTier::High}) {
        expect(validQualitySettings(qualityPreset(tier)), "quality presets are bounded");
    }
    auto bad = qualityPreset(QualityTier::Balanced);
    bad.renderScale = std::numeric_limits<float>::quiet_NaN();
    expect(!validQualitySettings(bad), "NaN render scale rejected");
    bad = qualityPreset(QualityTier::High); bad.requestedMsaa = 3;
    expect(!validQualitySettings(bad), "invalid MSAA count rejected");
    constexpr std::array roles{
        MaterialRole::PowderConcrete, MaterialRole::IndustrialDark,
        MaterialRole::CrtCyan, MaterialRole::SignalMagenta,
        MaterialRole::SodiumAmber, MaterialRole::VoidBlack,
        MaterialRole::TerminalGreen, MaterialRole::HazardRed
    };
    for (auto role : roles) {
        const auto style = materialStyle(role);
        expect(in01(style.roughness) && in01(style.metallic) && in01(style.emission),
               "material coefficients bounded");
        for (const auto n : {Color3{1,0,0}, Color3{-1,0,0}, Color3{0,1,0},
                             Color3{0,-1,0}, Color3{0,0,1}, Color3{0,0,-1}}) {
            const auto lit = shadeDebugSurface(role, n);
            expect(in01(lit.r) && in01(lit.g) && in01(lit.b), "lit faces display-safe");
        }
    }
    auto top = shadeDebugSurface(MaterialRole::PowderConcrete, {0,1,0});
    auto bottom = shadeDebugSurface(MaterialRole::PowderConcrete, {0,-1,0});
    expect(top.r > bottom.r, "directional light changes surface illumination");
    auto bright = shadeDebugSurface(MaterialRole::CrtCyan, {0,1,0});
    auto dark = shadeDebugSurface(MaterialRole::VoidBlack, {0,1,0});
    expect(bright.g > dark.g + 0.25f, "signals readable against dark void");
    LightingSettings badLight{};
    badLight.exposure = std::numeric_limits<float>::infinity();
    auto safe = shadeDebugSurface(MaterialRole::PowderConcrete, {0,0,0}, badLight);
    expect(in01(safe.r) && in01(safe.g) && in01(safe.b), "nonfinite light safe");
    auto pause = clearColor(true), active = clearColor(false);
    expect(pause.r != active.r, "pause and active clear states distinct");
    std::cout << "[PASS] INOKUI G1 material, lighting and quality contracts\n";
}
