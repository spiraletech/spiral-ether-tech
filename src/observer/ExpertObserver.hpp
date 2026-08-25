#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "input/HakuiInput.hpp"
#include "social/ChatSystem.hpp"
#include "world/WorldGeometry.hpp"

namespace hakui::observer {

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct BuildObservation {
    std::string hakuiVersion = "0.861-dev";
    std::string configuration = "unknown";
    std::string gitCommit = "unknown";
    std::string gitBranch = "unknown";
    std::string buildTimestamp = "unknown";
    std::string platform = "unknown";
    std::string rendererBackend = "unknown";
};

struct AttachmentObservation {
    std::string semantic;
    std::string parent;
    Vec3 localPosition{};
};

struct EntityObservation {
    std::uint32_t id = 0;
    std::string type;
    std::string parent;
    bool active = true;
    Vec3 position{};
    Vec3 velocity{};
    float yaw = 0.0f;
    bool grounded = false;
    std::string locomotion;
    std::string movementState;
    std::string animationState;
    std::string combatState;
    float health = 0.0f;
    float stamina = 0.0f;
    float balance = 0.0f;
    std::string currentRideable;
    std::string currentInteraction;
    std::string currentWorldZone;
    std::vector<AttachmentObservation> attachments;
    std::string rideDiscipline = "NONE";
    std::string rideState = "INACTIVE";
    std::string currentTrick = "READY";
    std::string rotationChannel = "NONE";
    std::string landingQuality = "--";
    std::string bailReason = "NONE";
    float popPreload = 0.0f;
    float popImpulse = 0.0f;
    float airtime = 0.0f;
    Vec3 rideableRotation{};
    Vec3 angularVelocity{};
    Vec3 targetRotation{};
    float rotationCompletion = 1.0f;
    float leftHandGripError = 0.0f;
    float rightHandGripError = 0.0f;
    float leftFootAnchorError = 0.0f;
    float rightFootAnchorError = 0.0f;
    std::string skateStance = "REGULAR";
    std::string footContactState = "ANCHORED";
    std::string airPose = "NONE";
    float pelvisYawRelativeToBoard = 0.0f;
    float frontFootAnchorError = 0.0f;
    float rearFootAnchorError = 0.0f;
    float leftKneeFlex = 0.0f;
    float rightKneeFlex = 0.0f;
    float preloadPoseWeight = 0.0f;
    float landingCompression = 0.0f;
    std::uint32_t seatAnchorId = 0;
    float seatAnchorError = 0.0f;
    bool seatOccupancy = false;
};

struct CameraObservation {
    std::string mode = "gameplay_follow";
    Vec3 position{};
    Vec3 target{};
    std::string targetEntity = "player.1";
    float yaw = 0.0f;
    float pitch = 0.0f;
    float distance = 0.0f;
    float fieldOfViewDegrees = 60.0f;
    bool orbiting = false;
    std::string inputOwner;
};

struct RuntimeObservation {
    float elapsedSeconds = 0.0f;
    bool paused = false;
    std::uint64_t worldRevision = 0;
    std::vector<std::string> recentEvents;
};

struct RideControlObservation {
    std::string activeDiscipline = "ON_FOOT";
    bool popIntent = false;
    bool airborne = false;
    bool trickWindowArmed = false;
    float trickWindowRemaining = 0.0f;
    float rawRightStickX = 0.0f;
    float rawRightStickY = 0.0f;
    float normalizedFlickX = 0.0f;
    float normalizedFlickY = 0.0f;
    std::string detectedFlick = "NONE";
    bool trickIntent = false;
    bool cameraOwnsRightStick = true;
    bool grindIntent = false;
    bool balanceIntent = false;
    float propulsion = 0.0f;
    bool spinLeft = false;
    bool spinRight = false;
    std::string rightStickOwner = "CAMERA";
    std::string rideState = "INACTIVE";
    float cameraDeadzone = 0.0f;
    float trickWindowDelay = 0.0f;
    float trickWindowDuration = 0.0f;
    float flickDeadzone = 0.0f;
    float flickThreshold = 0.0f;
    float flickReleaseThreshold = 0.0f;
    bool popPreparing = false;
    float popPreload = 0.0f;
    float preloadSaturationTime = 0.0f;
};

struct SeatAnchorObservation {
    std::uint32_t id = 0;
    std::uint32_t furnitureAffordanceId = 0;
    std::string label;
    Vec3 localPosition{};
    float localYaw = 0.0f;
    Vec3 worldPosition{};
    float worldYaw = 0.0f;
    bool occupied = false;
    std::string poseProfile;
};

struct SocialObservation {
    std::string inputOwner = "GameplayInput";
    bool chatInputActive = false;
    std::string chatBuffer;
    std::size_t recentMessageCount = 0;
    std::uint64_t lastMessageId = 0;
    std::string lastMessageText;
    std::uint32_t lastSpeakerId = 0;
    std::string speechIntent = "Neutral";
    bool bubbleActive = false;
    float bubbleRemaining = 0.0f;
    Vec3 bubbleAnchorPosition{};
    Vec3 bubbleWorldPosition{};
    Vec3 bubbleScreenPosition{};
    float bubbleScale = 0.0f;
    float bubbleAlpha = 0.0f;
    float bubbleWidth = 0.0f;
    float bubbleHeight = 0.0f;
    std::size_t bubbleLineCount = 0;
    std::string bubbleStyleProfile = "human.local.glass";
    std::string bubbleLifePhase = "Hidden";
    float bubbleDistanceToCamera = 0.0f;
    float bubbleAnchorError = 0.0f;
    std::string currentSocialGesture = "None";
    std::string lastChannel = "System";
    std::string lastMessageSource = "SystemAI";
    std::string bubbleStyle = "LocalSpeech";
    std::string environmentModifier = "None";
};

struct CaptureContext {
    BuildObservation build;
    std::string worldVersion = "black-room.v0.861";
    std::string environmentId = "data-grunge.black-room";
    std::span<const WorldPrimitive> geometry;
    std::span<const WorldAffordanceVolume> affordances;
    std::vector<SeatAnchorObservation> seatAnchors;
    Vec3 spawn{};
    float voidResetHeight = -12.0f;
    std::vector<EntityObservation> entities;
    input::InputFrame input;
    std::uint32_t connectedGamepads = 0;
    bool rideEligible = false;
    RideControlObservation rideControl;
    SocialObservation social;
    std::string currentInteractionIntent = "none";
    CameraObservation camera;
    RuntimeObservation runtime;
};

class RuntimeEventJournal {
public:
    explicit RuntimeEventJournal(std::size_t capacity = 96);
    void record(float elapsedSeconds, std::string_view category, std::string_view message);
    std::vector<std::string> entries() const;

private:
    std::size_t capacity_ = 96;
    std::vector<std::string> entries_;
};

using FrameWriter = std::function<bool(
    const std::filesystem::path& destination,
    std::string& error
)>;

struct ExportResult {
    bool success = false;
    std::filesystem::path bundlePath;
    std::string captureId;
    std::string error;
};

class ExpertObserver {
public:
    static ExportResult capture(
        const CaptureContext& context,
        const std::filesystem::path& outputRoot,
        const FrameWriter& frameWriter
    );
    static std::string makeCaptureId();
};

} // namespace hakui::observer
