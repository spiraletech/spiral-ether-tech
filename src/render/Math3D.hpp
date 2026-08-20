#pragma once

#include <cmath>

namespace hakui::math {

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

inline Vec3 operator-(const Vec3& a, const Vec3& b)
{
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

inline float dot(const Vec3& a, const Vec3& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline Vec3 cross(const Vec3& a, const Vec3& b)
{
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

inline Vec3 normalize(const Vec3& value)
{
    const float length = std::sqrt(dot(value, value));
    if (length <= 0.00001f) {
        return {};
    }

    return {value.x / length, value.y / length, value.z / length};
}

inline Vec3 cameraRelativePlanarMovement(float right, float forward, float cameraYaw)
{
    const float sine = std::sin(cameraYaw);
    const float cosine = std::cos(cameraYaw);
    return {
        -right * cosine - forward * sine,
        0.0f,
        right * sine - forward * cosine
    };
}

struct Mat4 {
    float m[16]{};
};

inline Mat4 identity()
{
    Mat4 out{};
    out.m[0] = 1.0f;
    out.m[5] = 1.0f;
    out.m[10] = 1.0f;
    out.m[15] = 1.0f;
    return out;
}

inline Mat4 multiply(const Mat4& lhs, const Mat4& rhs)
{
    Mat4 out{};

    for (int column = 0; column < 4; ++column) {
        for (int row = 0; row < 4; ++row) {
            float value = 0.0f;
            for (int k = 0; k < 4; ++k) {
                value += lhs.m[k * 4 + row] * rhs.m[column * 4 + k];
            }
            out.m[column * 4 + row] = value;
        }
    }

    return out;
}

inline Mat4 translation(const Vec3& value)
{
    Mat4 out = identity();
    out.m[12] = value.x;
    out.m[13] = value.y;
    out.m[14] = value.z;
    return out;
}

inline Mat4 scale(const Vec3& value)
{
    Mat4 out = identity();
    out.m[0] = value.x;
    out.m[5] = value.y;
    out.m[10] = value.z;
    return out;
}

inline Mat4 rotationX(float radians)
{
    Mat4 out = identity();
    const float cosine = std::cos(radians);
    const float sine = std::sin(radians);
    out.m[5] = cosine;
    out.m[6] = sine;
    out.m[9] = -sine;
    out.m[10] = cosine;
    return out;
}

inline Mat4 rotationY(float radians)
{
    Mat4 out = identity();
    const float cosine = std::cos(radians);
    const float sine = std::sin(radians);
    out.m[0] = cosine;
    out.m[2] = -sine;
    out.m[8] = sine;
    out.m[10] = cosine;
    return out;
}

inline Mat4 rotationZ(float radians)
{
    Mat4 out = identity();
    const float cosine = std::cos(radians);
    const float sine = std::sin(radians);
    out.m[0] = cosine;
    out.m[1] = sine;
    out.m[4] = -sine;
    out.m[5] = cosine;
    return out;
}

inline Mat4 perspectiveLH(float fovRadians, float aspect, float nearPlane, float farPlane)
{
    Mat4 out{};
    const float f = 1.0f / std::tan(fovRadians * 0.5f);

    out.m[0] = f / aspect;
    out.m[5] = f;
    out.m[10] = farPlane / (farPlane - nearPlane);
    out.m[11] = 1.0f;
    out.m[14] = (-nearPlane * farPlane) / (farPlane - nearPlane);
    return out;
}

inline Mat4 lookAtLH(const Vec3& eye, const Vec3& target, const Vec3& up)
{
    const Vec3 forward = normalize(target - eye);
    const Vec3 right = normalize(cross(up, forward));
    const Vec3 cameraUp = cross(forward, right);

    Mat4 out = identity();

    out.m[0] = right.x;
    out.m[4] = right.y;
    out.m[8] = right.z;
    out.m[12] = -dot(right, eye);

    out.m[1] = cameraUp.x;
    out.m[5] = cameraUp.y;
    out.m[9] = cameraUp.z;
    out.m[13] = -dot(cameraUp, eye);

    out.m[2] = forward.x;
    out.m[6] = forward.y;
    out.m[10] = forward.z;
    out.m[14] = -dot(forward, eye);

    return out;
}

} // namespace hakui::math
