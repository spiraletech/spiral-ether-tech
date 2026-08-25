#include "observer/ExpertObserver.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <system_error>

namespace hakui::observer {

namespace {

std::string jsonEscape(std::string_view value)
{
    std::string escaped;
    escaped.reserve(value.size() + 8);
    for (const char character : value) {
        switch (character) {
            case '"': escaped += "\\\""; break;
            case '\\': escaped += "\\\\"; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default:
                if (static_cast<unsigned char>(character) >= 0x20) {
                    escaped += character;
                }
                break;
        }
    }
    return escaped;
}

std::string xmlEscape(std::string_view value)
{
    std::string escaped;
    for (const char character : value) {
        switch (character) {
            case '&': escaped += "&amp;"; break;
            case '<': escaped += "&lt;"; break;
            case '>': escaped += "&gt;"; break;
            case '"': escaped += "&quot;"; break;
            default: escaped += character; break;
        }
    }
    return escaped;
}

bool writeText(
    const std::filesystem::path& path,
    std::string_view text,
    std::string& error
)
{
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream) {
        error = "could not open " + path.string();
        return false;
    }
    stream.write(text.data(), static_cast<std::streamsize>(text.size()));
    if (!stream) {
        error = "could not write " + path.string();
        return false;
    }
    return true;
}

std::string_view primitiveName(WorldPrimitiveKind kind) noexcept
{
    switch (kind) {
        case WorldPrimitiveKind::Floor: return "Floor";
        case WorldPrimitiveKind::Wall: return "Wall";
        case WorldPrimitiveKind::Platform: return "Platform";
        case WorldPrimitiveKind::Ramp: return "Ramp";
        case WorldPrimitiveKind::Furniture: return "Furniture";
        case WorldPrimitiveKind::Casino: return "Casino";
        case WorldPrimitiveKind::Terminal: return "Terminal";
        case WorldPrimitiveKind::Mobility: return "Mobility";
        case WorldPrimitiveKind::Signage: return "Signage";
        case WorldPrimitiveKind::Monument: return "Monument";
        case WorldPrimitiveKind::VoidMarker: return "VoidMarker";
    }
    return "Unknown";
}

std::string_view materialName(MaterialRole material) noexcept
{
    switch (material) {
        case MaterialRole::PowderConcrete: return "PowderConcrete";
        case MaterialRole::IndustrialDark: return "IndustrialDark";
        case MaterialRole::CrtCyan: return "CrtCyan";
        case MaterialRole::SignalMagenta: return "SignalMagenta";
        case MaterialRole::SodiumAmber: return "SodiumAmber";
        case MaterialRole::VoidBlack: return "VoidBlack";
        case MaterialRole::TerminalGreen: return "TerminalGreen";
        case MaterialRole::HazardRed: return "HazardRed";
    }
    return "Unknown";
}

constexpr std::array<std::pair<WorldAffordance, std::string_view>, 19>
kAffordanceNames{{
    {WorldAffordance::Rideable, "Rideable"},
    {WorldAffordance::Grindable, "Grindable"},
    {WorldAffordance::Transition, "Transition"},
    {WorldAffordance::Launch, "Launch"},
    {WorldAffordance::Landing, "Landing"},
    {WorldAffordance::ManualZone, "ManualZone"},
    {WorldAffordance::StallAnchor, "StallAnchor"},
    {WorldAffordance::Seat, "Seat"},
    {WorldAffordance::CasinoAnchor, "CasinoAnchor"},
    {WorldAffordance::Terminal, "Terminal"},
    {WorldAffordance::FightZone, "FightZone"},
    {WorldAffordance::SparAnchor, "SparAnchor"},
    {WorldAffordance::SpectatorZone, "SpectatorZone"},
    {WorldAffordance::RespawnVolume, "RespawnVolume"},
    {WorldAffordance::Void, "Void"},
    {WorldAffordance::DuelZone, "DuelZone"},
    {WorldAffordance::ArcheryLane, "ArcheryLane"},
    {WorldAffordance::WeaponRack, "WeaponRack"},
    {WorldAffordance::Target, "Target"}
}};

std::string vec3Json(const Vec3& value)
{
    std::ostringstream output;
    output << std::fixed << std::setprecision(4)
           << "{\"x\":" << value.x
           << ",\"y\":" << value.y
           << ",\"z\":" << value.z << '}';
    return output.str();
}

std::string buildJson(const BuildObservation& build)
{
    std::ostringstream output;
    output << "{\n"
           << "  \"hakui_version\": \"" << jsonEscape(build.hakuiVersion) << "\",\n"
           << "  \"build_configuration\": \"" << jsonEscape(build.configuration) << "\",\n"
           << "  \"git_commit\": \"" << jsonEscape(build.gitCommit) << "\",\n"
           << "  \"git_branch\": \"" << jsonEscape(build.gitBranch) << "\",\n"
           << "  \"build_timestamp\": \"" << jsonEscape(build.buildTimestamp) << "\",\n"
           << "  \"platform\": \"" << jsonEscape(build.platform) << "\",\n"
           << "  \"renderer_backend\": \"" << jsonEscape(build.rendererBackend) << "\"\n"
           << "}\n";
    return output.str();
}

std::string worldJson(const CaptureContext& context)
{
    std::ostringstream output;
    output << std::fixed << std::setprecision(4)
           << "{\n  \"world_version\": \"" << jsonEscape(context.worldVersion)
           << "\",\n  \"environment_id\": \"" << jsonEscape(context.environmentId)
           << "\",\n  \"spawn\": " << vec3Json(context.spawn)
           << ",\n  \"void_reset_height\": " << context.voidResetHeight
           << ",\n  \"objects\": [\n";
    for (std::size_t index = 0; index < context.geometry.size(); ++index) {
        const WorldPrimitive& object = context.geometry[index];
        output << "    {\"id\":\"primitive." << std::setw(4) << std::setfill('0')
               << index + 1 << std::setfill(' ') << "\",\"semantic_type\":\""
               << primitiveName(object.kind) << "\",\"position\":{"
               << "\"x\":" << object.x << ",\"y\":" << object.y
               << ",\"z\":" << object.z << "},\"rotation\":{"
               << "\"x\":" << object.rotationX << ",\"y\":" << object.rotationY
               << ",\"z\":" << object.rotationZ << "},\"scale\":{"
               << "\"x\":" << object.width << ",\"y\":" << object.height
               << ",\"z\":" << object.depth << "},\"bounds\":{"
               << "\"min_x\":" << object.x - object.width * 0.5f
               << ",\"max_x\":" << object.x + object.width * 0.5f
               << ",\"min_y\":" << object.y - object.height * 0.5f
               << ",\"max_y\":" << object.y + object.height * 0.5f
               << ",\"min_z\":" << object.z - object.depth * 0.5f
               << ",\"max_z\":" << object.z + object.depth * 0.5f
               << "},\"material_role\":\"" << materialName(object.material)
               << "\",\"collision_role\":\""
               << (object.kind == WorldPrimitiveKind::VoidMarker ? "void" : "authored_geometry")
               << "\",\"repeat\":{" << "\"count\":" << object.repeatCount
               << ",\"x\":" << object.repeatX << ",\"y\":" << object.repeatY
               << ",\"z\":" << object.repeatZ << "}}"
               << (index + 1 == context.geometry.size() ? "\n" : ",\n");
    }
    output << "  ],\n  \"affordance_volumes\": [\n";
    for (std::size_t index = 0; index < context.affordances.size(); ++index) {
        const WorldAffordanceVolume& volume = context.affordances[index];
        output << "    {\"id\":" << volume.id << ",\"label\":\""
               << jsonEscape(volume.label) << "\",\"bounds\":{"
               << "\"min_x\":" << volume.minimumX << ",\"max_x\":" << volume.maximumX
               << ",\"min_y\":" << volume.minimumY << ",\"max_y\":" << volume.maximumY
               << ",\"min_z\":" << volume.minimumZ << ",\"max_z\":" << volume.maximumZ
               << "},\"affordances\":[";
        bool first = true;
        for (const auto& [affordance, name] : kAffordanceNames) {
            if (hasAffordance(volume.affordances, affordance)) {
                output << (first ? "" : ",") << '"' << name << '"';
                first = false;
            }
        }
        output << "],\"primary_anchor\":{" << "\"x\":" << volume.primaryAnchor.x
               << ",\"y\":" << volume.primaryAnchor.y << ",\"z\":" << volume.primaryAnchor.z
               << ",\"yaw\":" << volume.primaryAnchor.yaw << "},\"secondary_anchor\":{"
               << "\"x\":" << volume.secondaryAnchor.x << ",\"y\":" << volume.secondaryAnchor.y
               << ",\"z\":" << volume.secondaryAnchor.z << ",\"yaw\":" << volume.secondaryAnchor.yaw
               << "}}" << (index + 1 == context.affordances.size() ? "\n" : ",\n");
    }
    output << "  ],\n  \"seat_anchors\": [\n";
    for (std::size_t index = 0; index < context.seatAnchors.size(); ++index) {
        const SeatAnchorObservation& seat = context.seatAnchors[index];
        output << "    {\"id\":" << seat.id
               << ",\"furniture_affordance_id\":"
               << seat.furnitureAffordanceId
               << ",\"label\":\"" << jsonEscape(seat.label)
               << "\",\"local_position\":" << vec3Json(seat.localPosition)
               << ",\"local_yaw\":" << seat.localYaw
               << ",\"world_position\":" << vec3Json(seat.worldPosition)
               << ",\"world_yaw\":" << seat.worldYaw
               << ",\"occupied\":" << (seat.occupied ? "true" : "false")
               << ",\"pose_profile\":\"" << jsonEscape(seat.poseProfile)
               << "\"}"
               << (index + 1 == context.seatAnchors.size() ? "\n" : ",\n");
    }
    output << "  ]\n}\n";
    return output.str();
}

std::string entitiesJson(const std::vector<EntityObservation>& entities)
{
    std::ostringstream output;
    output << std::fixed << std::setprecision(4) << "{\n  \"entities\": [\n";
    for (std::size_t index = 0; index < entities.size(); ++index) {
        const EntityObservation& entity = entities[index];
        output << "    {\"id\":" << entity.id << ",\"type\":\"" << jsonEscape(entity.type)
               << "\",\"parent\":\"" << jsonEscape(entity.parent) << "\",\"active\":"
               << (entity.active ? "true" : "false") << ",\"position\":" << vec3Json(entity.position)
               << ",\"orientation_yaw\":" << entity.yaw << ",\"velocity\":" << vec3Json(entity.velocity)
               << ",\"grounded\":" << (entity.grounded ? "true" : "false")
               << ",\"locomotion\":\"" << jsonEscape(entity.locomotion)
               << "\",\"movement_state\":\"" << jsonEscape(entity.movementState)
               << "\",\"animation_state\":\"" << jsonEscape(entity.animationState)
               << "\",\"combat_state\":\"" << jsonEscape(entity.combatState)
               << "\",\"health\":" << entity.health << ",\"stamina\":" << entity.stamina
               << ",\"balance\":" << entity.balance << ",\"current_rideable\":\""
               << jsonEscape(entity.currentRideable) << "\",\"current_interaction\":\""
               << jsonEscape(entity.currentInteraction) << "\",\"current_world_zone\":\""
               << jsonEscape(entity.currentWorldZone)
               << "\",\"ride_physics\":{\"ride_discipline\":\""
               << jsonEscape(entity.rideDiscipline)
               << "\",\"ride_state\":\"" << jsonEscape(entity.rideState)
               << "\",\"pop_preload\":" << entity.popPreload
               << ",\"pop_impulse\":" << entity.popImpulse
               << ",\"airtime\":" << entity.airtime
               << ",\"current_trick\":\"" << jsonEscape(entity.currentTrick)
               << "\",\"rotation_channel\":\"" << jsonEscape(entity.rotationChannel)
               << "\",\"rideable_rotation\":" << vec3Json(entity.rideableRotation)
               << ",\"angular_velocity\":" << vec3Json(entity.angularVelocity)
               << ",\"target_rotation\":" << vec3Json(entity.targetRotation)
               << ",\"rotation_completion\":" << entity.rotationCompletion
               << ",\"landing_quality\":\"" << jsonEscape(entity.landingQuality)
               << "\",\"bail_reason\":\"" << jsonEscape(entity.bailReason)
               << "\",\"left_hand_grip_error\":" << entity.leftHandGripError
               << ",\"right_hand_grip_error\":" << entity.rightHandGripError
               << ",\"left_foot_anchor_error\":" << entity.leftFootAnchorError
               << ",\"right_foot_anchor_error\":" << entity.rightFootAnchorError
               << "},\"body_mechanics\":{\"skate_stance\":\""
               << jsonEscape(entity.skateStance)
               << "\",\"foot_contact_state\":\""
               << jsonEscape(entity.footContactState)
               << "\",\"air_pose\":\"" << jsonEscape(entity.airPose)
               << "\",\"pelvis_yaw_relative_to_board\":"
               << entity.pelvisYawRelativeToBoard
               << ",\"front_foot_anchor_error\":" << entity.frontFootAnchorError
               << ",\"rear_foot_anchor_error\":" << entity.rearFootAnchorError
               << ",\"left_knee_flex\":" << entity.leftKneeFlex
               << ",\"right_knee_flex\":" << entity.rightKneeFlex
               << ",\"preload_pose_weight\":" << entity.preloadPoseWeight
               << ",\"landing_compression\":" << entity.landingCompression
               << "},\"seat_state\":{\"seat_anchor_id\":" << entity.seatAnchorId
               << ",\"seat_anchor_error\":" << entity.seatAnchorError
               << ",\"seat_occupancy\":"
               << (entity.seatOccupancy ? "true" : "false")
               << "},\"attachments\":[";
        for (std::size_t attachmentIndex = 0;
             attachmentIndex < entity.attachments.size(); ++attachmentIndex) {
            const AttachmentObservation& attachment = entity.attachments[attachmentIndex];
            output << "{\"semantic\":\"" << jsonEscape(attachment.semantic)
                   << "\",\"parent\":\"" << jsonEscape(attachment.parent)
                   << "\",\"local_position\":" << vec3Json(attachment.localPosition) << '}'
                   << (attachmentIndex + 1 == entity.attachments.size() ? "" : ",");
        }
        output << "]}" << (index + 1 == entities.size() ? "\n" : ",\n");
    }
    output << "  ]\n}\n";
    return output.str();
}

std::string inputJson(const CaptureContext& context)
{
    const input::InputDevice activeDevice = context.input.activeDevice;
    std::ostringstream output;
    output << "{\n  \"active_input_device\": \""
           << input::InputResolver::deviceName(activeDevice)
           << "\",\n  \"input_ownership\": \""
           << jsonEscape(context.social.inputOwner)
           << "\",\n  \"controller_layout\": \""
           << input::InputResolver::controllerLayoutName(
               context.input.controllerLayout
           )
           << "\",\n  \"connected_gamepads\": " << context.connectedGamepads
           << ",\n  \"gamepad_available\": "
           << (context.input.gamepadAvailable ? "true" : "false")
           << ",\n  \"ride_eligible\": " << (context.rideEligible ? "true" : "false")
           << ",\n  \"current_interaction_intent\": \""
           << jsonEscape(context.currentInteractionIntent)
           << "\",\n  \"hot_swap_state\": {\"connected_this_frame\":"
           << (context.input.gamepadConnected ? "true" : "false")
           << ",\"disconnected_this_frame\":"
           << (context.input.gamepadDisconnected ? "true" : "false")
           << "},\n  \"ride_control\": {\n"
           << "    \"active_discipline\": \""
           << jsonEscape(context.rideControl.activeDiscipline)
           << "\",\n    \"pop_intent\": "
           << (context.rideControl.popIntent ? "true" : "false")
           << ",\n    \"pop_preparing\": "
           << (context.rideControl.popPreparing ? "true" : "false")
           << ",\n    \"pop_preload\": " << std::fixed
           << std::setprecision(4) << context.rideControl.popPreload
           << ",\n    \"airborne\": "
           << (context.rideControl.airborne ? "true" : "false")
           << ",\n    \"trick_window_armed\": "
           << (context.rideControl.trickWindowArmed ? "true" : "false")
           << ",\n    \"trick_window_remaining\": " << std::fixed
           << std::setprecision(4) << context.rideControl.trickWindowRemaining
           << ",\n    \"raw_right_stick\": {\"x\":" << std::fixed
           << std::setprecision(4) << context.rideControl.rawRightStickX
           << ",\"y\":" << context.rideControl.rawRightStickY
           << "},\n    \"normalized_flick_vector\": {\"x\":"
           << context.rideControl.normalizedFlickX << ",\"y\":"
           << context.rideControl.normalizedFlickY
           << "},\n    \"detected_flick\": \""
           << jsonEscape(context.rideControl.detectedFlick)
           << "\",\n    \"trick_intent\": "
           << (context.rideControl.trickIntent ? "true" : "false")
           << ",\n    \"camera_owns_right_stick\": "
           << (context.rideControl.cameraOwnsRightStick ? "true" : "false")
           << ",\n    \"grind_intent\": "
           << (context.rideControl.grindIntent ? "true" : "false")
           << ",\n    \"balance_intent\": "
           << (context.rideControl.balanceIntent ? "true" : "false")
           << ",\n    \"propulsion\": " << context.rideControl.propulsion
           << ",\n    \"spin_left\": "
           << (context.rideControl.spinLeft ? "true" : "false")
           << ",\n    \"spin_right\": "
           << (context.rideControl.spinRight ? "true" : "false")
           << ",\n    \"right_stick_owner\": \""
           << jsonEscape(context.rideControl.rightStickOwner)
           << "\",\n    \"ride_state\": \""
           << jsonEscape(context.rideControl.rideState)
           << "\",\n    \"tuning\": {\"camera_deadzone\":"
           << context.rideControl.cameraDeadzone
           << ",\"preload_saturation_time\":"
           << context.rideControl.preloadSaturationTime
           << ",\"trick_window_delay\":"
           << context.rideControl.trickWindowDelay
           << ",\"trick_window_duration\":"
           << context.rideControl.trickWindowDuration
           << ",\"flick_deadzone\":" << context.rideControl.flickDeadzone
           << ",\"flick_threshold\":"
           << context.rideControl.flickThreshold
           << ",\"flick_release_threshold\":"
           << context.rideControl.flickReleaseThreshold
           << "}\n"
           << "  },\n  \"semantic_action_map\": [\n";
    for (std::size_t index = 0; index < input::actionCount; ++index) {
        const input::Action action = static_cast<input::Action>(index);
        const input::ActionState& state = context.input.action(action);
        output << "    {\"action\":\"" << input::InputResolver::actionName(action)
               << "\",\"keyboard_binding\":\""
               << input::InputResolver::prompt(action, input::InputDevice::KeyboardMouse)
               << "\",\"controller_binding\":\""
               << input::InputResolver::prompt(
                   action,
                   input::InputDevice::Gamepad,
                   context.input.controllerLayout
               )
               << "\",\"held\":" << (state.held ? "true" : "false")
               << ",\"pressed\":" << (state.pressed ? "true" : "false")
               << ",\"value\":" << std::fixed << std::setprecision(4) << state.value << '}'
               << (index + 1 == input::actionCount ? "\n" : ",\n");
    }
    output << "  ]\n}\n";
    return output.str();
}

std::string socialJson(const SocialObservation& social)
{
    std::ostringstream output;
    output << std::fixed << std::setprecision(4)
           << "{\n  \"input_owner\": \"" << jsonEscape(social.inputOwner)
           << "\",\n  \"chat_input_active\": "
           << (social.chatInputActive ? "true" : "false")
           << ",\n  \"chat_buffer\": \"" << jsonEscape(social.chatBuffer)
           << "\",\n  \"recent_message_count\": " << social.recentMessageCount
           << ",\n  \"last_message_id\": " << social.lastMessageId
           << ",\n  \"last_message_text\": \""
           << jsonEscape(social.lastMessageText)
           << "\",\n  \"last_speaker_id\": " << social.lastSpeakerId
           << ",\n  \"last_channel\": \"" << jsonEscape(social.lastChannel)
           << "\",\n  \"last_message_source\": \""
           << jsonEscape(social.lastMessageSource)
           << "\",\n  \"speech_intent\": \"" << jsonEscape(social.speechIntent)
           << "\",\n  \"bubble_active\": "
           << (social.bubbleActive ? "true" : "false")
           << ",\n  \"bubble_remaining\": " << social.bubbleRemaining
           << ",\n  \"bubble_anchor_position\": "
           << vec3Json(social.bubbleAnchorPosition)
           << ",\n  \"bubble_world_position\": "
           << vec3Json(social.bubbleWorldPosition)
           << ",\n  \"bubble_screen_position\": "
           << vec3Json(social.bubbleScreenPosition)
           << ",\n  \"bubble_scale\": " << social.bubbleScale
           << ",\n  \"bubble_alpha\": " << social.bubbleAlpha
           << ",\n  \"bubble_width\": " << social.bubbleWidth
           << ",\n  \"bubble_height\": " << social.bubbleHeight
           << ",\n  \"bubble_line_count\": " << social.bubbleLineCount
           << ",\n  \"bubble_style_profile\": \""
           << jsonEscape(social.bubbleStyleProfile)
           << "\",\n  \"bubble_life_phase\": \""
           << jsonEscape(social.bubbleLifePhase)
           << "\",\n  \"bubble_distance_to_camera\": "
           << social.bubbleDistanceToCamera
           << ",\n  \"bubble_anchor_error\": " << social.bubbleAnchorError
           << ",\n  \"bubble_style\": \"" << jsonEscape(social.bubbleStyle)
           << "\",\n  \"environment_modifier\": \""
           << jsonEscape(social.environmentModifier)
           << "\",\n  \"current_social_gesture\": \""
           << jsonEscape(social.currentSocialGesture) << "\"\n}\n";
    return output.str();
}

std::string cameraJson(const CameraObservation& camera)
{
    std::ostringstream output;
    output << std::fixed << std::setprecision(4)
           << "{\n  \"camera_mode\": \"" << jsonEscape(camera.mode)
           << "\",\n  \"world_position\": " << vec3Json(camera.position)
           << ",\n  \"target_position\": " << vec3Json(camera.target)
           << ",\n  \"target_entity\": \"" << jsonEscape(camera.targetEntity)
           << "\",\n  \"yaw\": " << camera.yaw
           << ",\n  \"pitch\": " << camera.pitch
           << ",\n  \"distance\": " << camera.distance
           << ",\n  \"fov_degrees\": " << camera.fieldOfViewDegrees
           << ",\n  \"orbit_state\": " << (camera.orbiting ? "true" : "false")
           << ",\n  \"input_ownership\": \"" << jsonEscape(camera.inputOwner) << "\"\n}\n";
    return output.str();
}

std::string runtimeJson(const RuntimeObservation& runtime)
{
    std::ostringstream output;
    output << std::fixed << std::setprecision(3)
           << "{\n  \"elapsed_seconds\": " << runtime.elapsedSeconds
           << ",\n  \"paused\": " << (runtime.paused ? "true" : "false")
           << ",\n  \"world_revision\": " << runtime.worldRevision
           << ",\n  \"recent_event_count\": " << runtime.recentEvents.size() << "\n}\n";
    return output.str();
}

std::string runtimeLog(const RuntimeObservation& runtime)
{
    std::ostringstream output;
    for (const std::string& event : runtime.recentEvents) {
        output << event << '\n';
    }
    return output.str();
}

std::string doctrineJson()
{
    return R"({
  "schema": "hakui.doctrine.v1",
  "third_person_first": true,
  "avatar_identity_priority": "high",
  "expressive_movement_priority": "high",
  "semantic_world_architecture": "required",
  "controller_native_ride_disciplines": true,
  "one_controller_grammar": true,
  "ride_trick_rhythm": "pop_then_airborne_flick",
  "data_grunge_identity": "required",
  "generic_engine_demo_aesthetic": "avoid",
  "reference_pillars": {
    "avatar_social_expression": "identity and rooms",
    "embodied_world_progression": "activities and physical presence",
    "multi_discipline_movement": "continuous on-foot, skateboard and BMX",
    "hakui": "original DATA GRUNGE synthesis"
  }
}
)";
}

std::string mapSvg(const CaptureContext& context)
{
    constexpr float width = 1200.0f;
    constexpr float height = 800.0f;
    constexpr float minimumX = -12.0f;
    constexpr float maximumX = 12.0f;
    constexpr float minimumZ = -10.0f;
    constexpr float maximumZ = 10.0f;
    const auto mapX = [=](float x) { return (x - minimumX) / (maximumX - minimumX) * width; };
    const auto mapY = [=](float z) { return height - (z - minimumZ) / (maximumZ - minimumZ) * height; };
    const auto color = [](WorldPrimitiveKind kind) -> std::string_view {
        switch (kind) {
            case WorldPrimitiveKind::Ramp: return "#ff9f1c";
            case WorldPrimitiveKind::Casino: return "#ff2ca8";
            case WorldPrimitiveKind::Terminal: return "#32f08c";
            case WorldPrimitiveKind::Furniture: return "#18d6e8";
            case WorldPrimitiveKind::VoidMarker: return "#05060a";
            default: return "#9297a3";
        }
    };
    std::ostringstream svg;
    svg << std::fixed << std::setprecision(2)
        << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"1200\" height=\"800\" viewBox=\"0 0 1200 800\">\n"
        << "<rect width=\"1200\" height=\"800\" fill=\"#080a10\"/>\n"
        << "<g stroke=\"#151a24\" stroke-width=\"1\">";
    for (int x = 0; x <= 1200; x += 50) svg << "<path d=\"M" << x << " 0V800\"/>";
    for (int y = 0; y <= 800; y += 50) svg << "<path d=\"M0 " << y << "H1200\"/>";
    svg << "</g>\n<g opacity=\"0.78\">\n";
    for (std::size_t index = 0; index < context.geometry.size(); ++index) {
        const WorldPrimitive& object = context.geometry[index];
        for (std::uint16_t repeat = 0;
             repeat < std::max<std::uint16_t>(object.repeatCount, 1);
             ++repeat) {
            const float objectX = object.x + object.repeatX * repeat;
            const float objectZ = object.z + object.repeatZ * repeat;
            const float x = mapX(objectX - object.width * 0.5f);
            const float y = mapY(objectZ + object.depth * 0.5f);
            const float w = object.width / (maximumX - minimumX) * width;
            const float h = object.depth / (maximumZ - minimumZ) * height;
            svg << "<rect id=\"primitive-" << index + 1 << "-instance-"
                << repeat + 1 << "\" x=\"" << x << "\" y=\"" << y
                << "\" width=\"" << std::max(w, 2.0f) << "\" height=\""
                << std::max(h, 2.0f) << "\" fill=\"" << color(object.kind)
                << "\"><title>" << primitiveName(object.kind)
                << "</title></rect>\n";
        }
    }
    svg << "</g>\n<g fill=\"none\" stroke-width=\"3\" stroke-dasharray=\"8 5\">\n";
    for (const WorldAffordanceVolume& volume : context.affordances) {
        std::string_view stroke = "#465066";
        if (hasAffordance(volume.affordances, WorldAffordance::FightZone)) stroke = "#ff334f";
        else if (hasAffordance(volume.affordances, WorldAffordance::CasinoAnchor)) stroke = "#ff2ca8";
        else if (hasAffordance(volume.affordances, WorldAffordance::Grindable)) stroke = "#19d9ed";
        else continue;
        svg << "<rect x=\"" << mapX(volume.minimumX) << "\" y=\"" << mapY(volume.maximumZ)
            << "\" width=\"" << (volume.maximumX - volume.minimumX) / (maximumX - minimumX) * width
            << "\" height=\"" << (volume.maximumZ - volume.minimumZ) / (maximumZ - minimumZ) * height
            << "\" stroke=\"" << stroke << "\"><title>" << xmlEscape(volume.label) << "</title></rect>\n";
    }
    svg << "</g>\n";
    for (const EntityObservation& entity : context.entities) {
        const bool player = entity.type == "PlayerAvatar" || entity.type == "Player";
        const bool rideable = entity.type == "Skateboard" || entity.type == "BMX";
        if (!player && !rideable) {
            continue;
        }
        svg << "<circle cx=\"" << mapX(entity.position.x) << "\" cy=\""
            << mapY(entity.position.z) << "\" r=\"" << (player ? 9 : 6)
            << "\" fill=\"" << (player ? "#ffffff" : "#ff9f1c")
            << "\" stroke=\"" << (player ? "#19d9ed" : "#ff2ca8")
            << "\" stroke-width=\"3\"><title>" << xmlEscape(entity.type)
            << "</title></circle>\n";
    }
    svg << "<text x=\"24\" y=\"36\" fill=\"#ffffff\" font-family=\"monospace\" font-size=\"22\">HAKUI EXPERT MAP // "
        << xmlEscape(context.environmentId) << "</text>\n</svg>\n";
    return svg.str();
}

} // namespace

RuntimeEventJournal::RuntimeEventJournal(std::size_t capacity)
    : capacity_(std::max<std::size_t>(capacity, 1))
{
}

void RuntimeEventJournal::record(
    float elapsedSeconds,
    std::string_view category,
    std::string_view message
)
{
    std::ostringstream line;
    line << '[' << std::fixed << std::setprecision(3) << elapsedSeconds << "] ["
         << category << "] " << message;
    if (entries_.size() == capacity_) {
        entries_.erase(entries_.begin());
    }
    entries_.push_back(line.str());
}

std::vector<std::string> RuntimeEventJournal::entries() const
{
    return entries_;
}

std::string ExpertObserver::makeCaptureId()
{
    const auto now = std::chrono::system_clock::now();
    const std::time_t timestamp = std::chrono::system_clock::to_time_t(now);
    std::tm local{};
#if defined(_WIN32)
    localtime_s(&local, &timestamp);
#else
    localtime_r(&timestamp, &local);
#endif
    std::ostringstream output;
    output << std::put_time(&local, "%Y-%m-%d_%H-%M-%S");
    return output.str();
}

ExportResult ExpertObserver::capture(
    const CaptureContext& context,
    const std::filesystem::path& outputRoot,
    const FrameWriter& frameWriter
)
{
    ExportResult result;
    result.captureId = makeCaptureId();
    result.bundlePath = outputRoot / result.captureId;
    std::error_code filesystemError;
    std::filesystem::create_directories(result.bundlePath, filesystemError);
    if (filesystemError) {
        result.error = "could not create observer bundle: " + filesystemError.message();
        return result;
    }

    const auto write = [&](std::string_view name, const std::string& content) {
        return writeText(result.bundlePath / name, content, result.error);
    };
    if (!write("BuildInfo.json", buildJson(context.build)) ||
        !write("WorldSnapshot.json", worldJson(context)) ||
        !write("EntitySnapshot.json", entitiesJson(context.entities)) ||
        !write("InputSnapshot.json", inputJson(context)) ||
        !write("SocialSnapshot.json", socialJson(context.social)) ||
        !write("CameraSnapshot.json", cameraJson(context.camera)) ||
        !write("RuntimeSnapshot.json", runtimeJson(context.runtime)) ||
        !write("Runtime.log", runtimeLog(context.runtime)) ||
        !write("MapSnapshot.svg", mapSvg(context)) ||
        !write("HakuiDoctrine.json", doctrineJson())) {
        return result;
    }

    if (!frameWriter ||
        !frameWriter(result.bundlePath / "FrameSnapshot.png", result.error)) {
        if (result.error.empty()) result.error = "frame writer unavailable";
        const std::string failureLog = runtimeLog(context.runtime) +
            "[observer.error] capture failed // " + result.error + "\n";
        std::string ignoredError;
        (void)writeText(result.bundlePath / "Runtime.log", failureLog, ignoredError);
        return result;
    }

    const std::string successLog = runtimeLog(context.runtime) +
        "[observer.capture] frame and semantic snapshot complete\n";
    if (!write("Runtime.log", successLog)) {
        return result;
    }

    std::ostringstream manifest;
    manifest << "{\n"
             << "  \"schema\": \"hakui.expert-observer.v1\",\n"
             << "  \"hakui_version\": \"" << jsonEscape(context.build.hakuiVersion) << "\",\n"
             << "  \"capture_id\": \"" << result.captureId << "\",\n"
             << "  \"authority\": \"read_only_observer\",\n"
             << "  \"files\": {\n"
             << "    \"build\": \"BuildInfo.json\",\n"
             << "    \"world\": \"WorldSnapshot.json\",\n"
             << "    \"entities\": \"EntitySnapshot.json\",\n"
             << "    \"input\": \"InputSnapshot.json\",\n"
             << "    \"social\": \"SocialSnapshot.json\",\n"
             << "    \"camera\": \"CameraSnapshot.json\",\n"
             << "    \"runtime_state\": \"RuntimeSnapshot.json\",\n"
             << "    \"map\": \"MapSnapshot.svg\",\n"
             << "    \"frame\": \"FrameSnapshot.png\",\n"
             << "    \"runtime_log\": \"Runtime.log\",\n"
             << "    \"doctrine\": \"HakuiDoctrine.json\"\n"
             << "  }\n}\n";
    if (!write("InspectionManifest.json", manifest.str())) {
        return result;
    }
    result.success = true;
    return result;
}

} // namespace hakui::observer
