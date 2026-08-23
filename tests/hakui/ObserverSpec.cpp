#include <array>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>

#include "observer/ExpertObserver.hpp"

namespace {

std::string readText(const std::filesystem::path& path)
{
    std::ifstream stream(path, std::ios::binary);
    return {
        std::istreambuf_iterator<char>(stream),
        std::istreambuf_iterator<char>()
    };
}

} // namespace

int main()
{
    using namespace hakui;
    using namespace hakui::observer;

    const std::filesystem::path outputRoot =
        std::filesystem::temp_directory_path() / "hakui-observer-spec-owned";
    std::error_code cleanupError;
    std::filesystem::remove_all(outputRoot, cleanupError);

    const std::array geometry{
        WorldPrimitive{
            WorldPrimitiveKind::Floor,
            MaterialRole::PowderConcrete,
            0.0f, -0.1f, 0.0f,
            8.0f, 0.2f, 8.0f
        }
    };
    const std::array affordances{
        WorldAffordanceVolume{
            7001,
            "FUSION TABLE",
            WorldAffordance::Seat | WorldAffordance::CasinoAnchor,
            -1.0f, 1.0f, -0.2f, 2.0f, -1.0f, 1.0f,
            {0.0f, 0.0f, -0.5f, 0.0f},
            {0.0f, 0.0f, 0.5f, 3.14159f}
        }
    };

    CaptureContext context;
    context.build = {
        "0.82-dev", "spec", "abc123", "codex/controller-overhaul-v0.82",
        "2026-08-23", "test", "none"
    };
    context.geometry = geometry;
    context.affordances = affordances;
    context.spawn = {0.0f, 0.0f, 2.0f};
    context.entities.push_back({
        1,
        "Player",
        "",
        true,
        {0.0f, 0.0f, 2.0f},
        {},
        0.0f,
        true,
        "ON FOOT",
        "idle",
        "idle",
        "INACTIVE",
        100.0f,
        100.0f,
        100.0f,
        "none",
        "none",
        "spawn",
        {{"LeftFootAnchor", "player.1", {-0.18f, 0.0f, 0.0f}}}
    });
    context.currentInteractionIntent = "INTERACT";
    context.input.controllerLayout = input::ControllerLayout::PlayStation;
    context.rideControl = {
        "SKATEBOARD", true, true,
        0.8f, -0.2f, 0.76f, -0.14f,
        "RIGHT", true, true, 0.8f, false, true,
        "TRICK_CAPTURE", "AIR",
        0.16f, 0.22f, 0.68f, 0.30f, 0.035f, 0.55f, 0.24f
    };
    context.camera.position = {2.0f, 3.0f, 6.0f};
    context.camera.target = {0.0f, 1.25f, 2.0f};
    context.runtime.recentEvents = {"[0.000] [boot] WORLD ONLINE"};

    const ExportResult result = ExpertObserver::capture(
        context,
        outputRoot,
        [](const std::filesystem::path& destination, std::string& error) {
            constexpr std::array<unsigned char, 8> pngSignature{
                0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a
            };
            std::ofstream stream(destination, std::ios::binary | std::ios::trunc);
            if (!stream) {
                error = "frame destination unavailable";
                return false;
            }
            stream.write(
                reinterpret_cast<const char*>(pngSignature.data()),
                static_cast<std::streamsize>(pngSignature.size())
            );
            return static_cast<bool>(stream);
        }
    );

    assert(result.success);
    constexpr std::array requiredFiles{
        "InspectionManifest.json",
        "BuildInfo.json",
        "WorldSnapshot.json",
        "EntitySnapshot.json",
        "InputSnapshot.json",
        "CameraSnapshot.json",
        "RuntimeSnapshot.json",
        "MapSnapshot.svg",
        "FrameSnapshot.png",
        "Runtime.log",
        "HakuiDoctrine.json"
    };
    for (const char* file : requiredFiles) {
        assert(std::filesystem::is_regular_file(result.bundlePath / file));
    }

    const std::string manifest = readText(result.bundlePath / "InspectionManifest.json");
    const std::string world = readText(result.bundlePath / "WorldSnapshot.json");
    const std::string input = readText(result.bundlePath / "InputSnapshot.json");
    const std::string map = readText(result.bundlePath / "MapSnapshot.svg");
    assert(manifest.find("hakui.expert-observer.v1") != std::string::npos);
    assert(manifest.find("read_only_observer") != std::string::npos);
    assert(world.find("primitive.0001") != std::string::npos);
    assert(world.find("CasinoAnchor") != std::string::npos);
    assert(input.find("semantic_action_map") != std::string::npos);
    assert(input.find("\"ride_control\"") != std::string::npos);
    assert(input.find("\"detected_flick\": \"RIGHT\"") != std::string::npos);
    assert(input.find("\"right_stick_owner\": \"TRICK_CAPTURE\"") != std::string::npos);
    assert(input.find("\"controller_layout\": \"PLAYSTATION-STYLE\"") != std::string::npos);
    assert(input.find("TRIANGLE") != std::string::npos);
    assert(input.find("CAPTURE EXPERT SNAPSHOT") != std::string::npos);
    assert(map.find("<svg") != std::string::npos);
    assert(map.find("FUSION TABLE") != std::string::npos);

    std::filesystem::remove_all(outputRoot, cleanupError);
    return 0;
}
