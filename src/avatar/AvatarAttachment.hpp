#pragma once

#include <string>

enum class AttachmentCategory {
    Body,
    Head,
    Hair,
    Face,
    Neck,
    Torso,
    Back,
    Waist,
    LeftHand,
    RightHand,
    LeftFoot,
    RightFoot,
    Skateboard,
    BMX,
    VehicleSeat
};

struct AvatarAttachment {
    std::string slot;
    std::string bone;
    AttachmentCategory category = AttachmentCategory::Body;
};
