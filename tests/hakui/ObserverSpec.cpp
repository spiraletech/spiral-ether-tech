#include <array>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>

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
        "0.861-dev", "spec", "abc123", "codex/social-bubble-visual-v0.861",
        "2026-08-23", "test", "none"
    };
    context.geometry = geometry;
    context.affordances = affordances;
    context.seatAnchors = {
        {100201, 7001, "VOID COUCH LEFT", {-0.82f, 0.0f, 0.0f},
         0.0f, {6.54f, 0.0f, 3.92f}, 3.14159f, false, "LoungeRelaxed"},
        {100202, 7001, "VOID COUCH RIGHT", {0.82f, 0.0f, 0.0f},
         0.0f, {4.90f, 0.0f, 3.92f}, 3.14159f, true, "LoungeRelaxed"}
    };
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
    EntityObservation rideable;
    rideable.id = 2002;
    rideable.type = "BMX";
    rideable.parent = "player.1";
    rideable.rideDiscipline = "BMX";
    rideable.rideState = "AIR";
    rideable.currentTrick = "BARSPIN";
    rideable.rotationChannel = "BMX_STEERING";
    rideable.landingQuality = "--";
    rideable.bailReason = "NONE";
    rideable.popPreload = 0.75f;
    rideable.popImpulse = 8.3125f;
    rideable.airtime = 0.42f;
    rideable.rideableRotation = {0.0f, 3.14159f, 0.0f};
    rideable.angularVelocity = {0.0f, 13.4f, 0.0f};
    rideable.targetRotation = {0.0f, 6.28318f, 0.0f};
    rideable.rotationCompletion = 0.5f;
    rideable.leftHandGripError = 0.0f;
    rideable.rightHandGripError = 0.0f;
    rideable.leftFootAnchorError = 0.12f;
    rideable.rightFootAnchorError = 0.12f;
    rideable.skateStance = "REGULAR";
    rideable.footContactState = "REACQUIRING";
    rideable.airPose = "BMX_TRICK";
    rideable.leftKneeFlex = 0.96f;
    rideable.rightKneeFlex = 0.96f;
    rideable.preloadPoseWeight = 0.75f;
    context.entities.push_back(std::move(rideable));
    context.currentInteractionIntent = "INTERACT";
    context.input.controllerLayout = input::ControllerLayout::PlayStation;
    context.rideControl = {
        "SKATEBOARD", true, true, true, 0.42f,
        0.8f, -0.2f, 0.76f, -0.14f,
        "RIGHT", true, false, true, true, 0.8f, false, true,
        "TRICK_WINDOW", "AIR",
        0.16f, 0.025f, 0.70f, 0.22f, 0.68f, 0.30f
    };
    context.rideControl.popPreparing = true;
    context.rideControl.popPreload = 0.75f;
    context.rideControl.preloadSaturationTime = 0.42f;
    context.social.inputOwner = "ChatInput";
    context.social.chatInputActive = true;
    context.social.chatBuffer = "hello";
    context.social.recentMessageCount = 2;
    context.social.lastMessageId = 42;
    context.social.lastMessageText = "hello?";
    context.social.lastSpeakerId = 1;
    context.social.speechIntent = "Question";
    context.social.bubbleActive = true;
    context.social.bubbleRemaining = 2.75f;
    context.social.bubbleAnchorPosition = {0.0f, 3.34f, 2.0f};
    context.social.bubbleWorldPosition = {0.0f, 4.15f, 2.0f};
    context.social.bubbleScreenPosition = {640.0f, 180.0f, 0.0f};
    context.social.bubbleScale = 1.02f;
    context.social.bubbleAlpha = 0.46f;
    context.social.bubbleWidth = 2.40f;
    context.social.bubbleHeight = 0.92f;
    context.social.bubbleLineCount = 2;
    context.social.bubbleStyleProfile = "human.local.glass";
    context.social.bubbleLifePhase = "Hold";
    context.social.bubbleDistanceToCamera = 8.5f;
    context.social.bubbleAnchorError = 0.0f;
    context.social.currentSocialGesture = "HeadTilt";
    context.social.lastChannel = "Local";
    context.social.lastMessageSource = "HumanPlayer";
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
        "SocialSnapshot.json",
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
    const std::string entities = readText(result.bundlePath / "EntitySnapshot.json");
    const std::string social = readText(result.bundlePath / "SocialSnapshot.json");
    const std::string map = readText(result.bundlePath / "MapSnapshot.svg");
    assert(manifest.find("hakui.expert-observer.v1") != std::string::npos);
    assert(manifest.find("read_only_observer") != std::string::npos);
    assert(world.find("primitive.0001") != std::string::npos);
    assert(world.find("CasinoAnchor") != std::string::npos);
    assert(world.find("\"seat_anchors\"") != std::string::npos);
    assert(world.find("VOID COUCH LEFT") != std::string::npos);
    assert(world.find("\"world_position\":{\"x\":6.5400") !=
           std::string::npos);
    assert(world.find("\"occupied\":true") != std::string::npos);
    assert(input.find("semantic_action_map") != std::string::npos);
    assert(input.find("\"input_ownership\": \"ChatInput\"") !=
           std::string::npos);
    assert(input.find("\"ride_control\"") != std::string::npos);
    assert(input.find("\"detected_flick\": \"RIGHT\"") != std::string::npos);
    assert(input.find("\"pop_intent\": true") != std::string::npos);
    assert(input.find("\"pop_preparing\": true") != std::string::npos);
    assert(input.find("\"pop_preload\": 0.7500") != std::string::npos);
    assert(input.find("\"trick_window_armed\": true") != std::string::npos);
    assert(input.find("\"trick_intent\": true") != std::string::npos);
    assert(input.find("\"camera_owns_right_stick\": false") != std::string::npos);
    assert(input.find("\"right_stick_owner\": \"TRICK_WINDOW\"") != std::string::npos);
    assert(input.find("activation_held") == std::string::npos);
    assert(input.find("capture_active") == std::string::npos);
    assert(input.find("\"controller_layout\": \"PLAYSTATION-STYLE\"") != std::string::npos);
    assert(input.find("TRIANGLE") != std::string::npos);
    assert(input.find("CAPTURE EXPERT SNAPSHOT") != std::string::npos);
    assert(entities.find("\"ride_discipline\":\"BMX\"") != std::string::npos);
    assert(entities.find("\"pop_impulse\":8.3125") != std::string::npos);
    assert(entities.find("\"rotation_channel\":\"BMX_STEERING\"") !=
           std::string::npos);
    assert(entities.find("\"left_hand_grip_error\":0.0000") !=
           std::string::npos);
    assert(entities.find("\"bail_reason\":\"NONE\"") != std::string::npos);
    assert(entities.find("\"body_mechanics\"") != std::string::npos);
    assert(entities.find("\"air_pose\":\"BMX_TRICK\"") != std::string::npos);
    assert(entities.find("\"preload_pose_weight\":0.7500") !=
           std::string::npos);
    assert(entities.find("\"seat_anchor_id\":0") != std::string::npos);
    assert(social.find("\"chat_input_active\": true") != std::string::npos);
    assert(social.find("\"chat_buffer\": \"hello\"") != std::string::npos);
    assert(social.find("\"last_message_id\": 42") != std::string::npos);
    assert(social.find("\"speech_intent\": \"Question\"") !=
           std::string::npos);
    assert(social.find("\"bubble_active\": true") != std::string::npos);
    assert(social.find("\"bubble_world_position\": {\"x\":0.0000,\"y\":4.1500") !=
           std::string::npos);
    assert(social.find("\"bubble_screen_position\": {\"x\":640.0000") !=
           std::string::npos);
    assert(social.find("\"bubble_scale\": 1.0200") != std::string::npos);
    assert(social.find("\"bubble_alpha\": 0.4600") != std::string::npos);
    assert(social.find("\"bubble_line_count\": 2") != std::string::npos);
    assert(social.find("\"bubble_style_profile\": \"human.local.glass\"") !=
           std::string::npos);
    assert(social.find("\"bubble_distance_to_camera\": 8.5000") !=
           std::string::npos);
    assert(social.find("\"bubble_anchor_error\": 0.0000") !=
           std::string::npos);
    assert(social.find("\"current_social_gesture\": \"HeadTilt\"") !=
           std::string::npos);
    assert(map.find("<svg") != std::string::npos);
    assert(map.find("FUSION TABLE") != std::string::npos);

    std::filesystem::remove_all(outputRoot, cleanupError);
    return 0;
}
