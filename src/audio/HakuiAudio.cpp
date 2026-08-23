#include "audio/HakuiAudio.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace {

constexpr int kSampleRate = 48000;
constexpr float kPi = 3.14159265358979323846f;

struct CueShape {
    float frequency;
    float seconds;
    float gain;
    float sweep;
    float grit;
};

CueShape cueShape(HakuiAudioCue cue)
{
    switch (cue) {
        case HakuiAudioCue::FootstepSoft: return {108.0f, 0.070f, 0.22f, -24.0f, 0.32f};
        case HakuiAudioCue::FootstepHard: return {82.0f, 0.085f, 0.28f, -20.0f, 0.46f};
        case HakuiAudioCue::Jump: return {205.0f, 0.120f, 0.22f, 230.0f, 0.08f};
        case HakuiAudioCue::Land: return {74.0f, 0.150f, 0.32f, -35.0f, 0.50f};
        case HakuiAudioCue::Interact: return {510.0f, 0.090f, 0.18f, 160.0f, 0.02f};
        case HakuiAudioCue::Casino: return {640.0f, 0.160f, 0.18f, 320.0f, 0.01f};
        case HakuiAudioCue::VoidRespawn: return {52.0f, 0.520f, 0.30f, 780.0f, 0.24f};
        case HakuiAudioCue::CombatSwing: return {190.0f, 0.095f, 0.20f, -90.0f, 0.22f};
        case HakuiAudioCue::CombatHit: return {76.0f, 0.115f, 0.34f, -18.0f, 0.72f};
        case HakuiAudioCue::CombatGuard: return {440.0f, 0.080f, 0.20f, -210.0f, 0.38f};
        case HakuiAudioCue::Knockdown: return {58.0f, 0.360f, 0.38f, -22.0f, 0.82f};
        case HakuiAudioCue::Recovery: return {140.0f, 0.220f, 0.20f, 260.0f, 0.12f};
        case HakuiAudioCue::RidePop: return {128.0f, 0.090f, 0.30f, 180.0f, 0.62f};
        case HakuiAudioCue::BoardRotation: return {310.0f, 0.120f, 0.16f, -140.0f, 0.44f};
        case HakuiAudioCue::BmxTrick: return {186.0f, 0.140f, 0.20f, 210.0f, 0.38f};
        case HakuiAudioCue::GrindScrape: return {92.0f, 0.260f, 0.22f, -18.0f, 0.92f};
        case HakuiAudioCue::CleanLanding: return {68.0f, 0.120f, 0.30f, -22.0f, 0.42f};
        case HakuiAudioCue::SketchyLanding: return {54.0f, 0.190f, 0.34f, 55.0f, 0.76f};
        case HakuiAudioCue::BailImpact: return {42.0f, 0.340f, 0.42f, -12.0f, 0.94f};
    }
    return {220.0f, 0.1f, 0.1f, 0.0f, 0.0f};
}

} // namespace

bool HakuiAudio::init()
{
    SDL_AudioSpec spec{};
    spec.format = SDL_AUDIO_F32;
    spec.channels = 1;
    spec.freq = kSampleRate;

    stream_ = SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
        &spec,
        nullptr,
        nullptr
    );
    if (!stream_) {
        SDL_LogWarn(
            SDL_LOG_CATEGORY_AUDIO,
            "[HAKUI] procedural audio unavailable: %s",
            SDL_GetError()
        );
        return false;
    }

    if (!SDL_ResumeAudioStreamDevice(stream_)) {
        SDL_LogWarn(
            SDL_LOG_CATEGORY_AUDIO,
            "[HAKUI] audio stream could not resume: %s",
            SDL_GetError()
        );
        SDL_DestroyAudioStream(stream_);
        stream_ = nullptr;
        return false;
    }

    SDL_Log("[HAKUI] procedural audio // footsteps + interaction cues online");
    return true;
}

void HakuiAudio::play(HakuiAudioCue cue)
{
    if (!stream_) {
        return;
    }

    const CueShape shape = cueShape(cue);
    const int sampleCount = std::max(1, static_cast<int>(shape.seconds * kSampleRate));
    std::vector<float> samples(static_cast<std::size_t>(sampleCount));
    float phase = 0.0f;
    for (int index = 0; index < sampleCount; ++index) {
        const float time = static_cast<float>(index) / static_cast<float>(kSampleRate);
        const float normalized = time / shape.seconds;
        const float envelope = (1.0f - normalized) * (1.0f - normalized);
        const float frequency = shape.frequency + shape.sweep * normalized;
        phase += 2.0f * kPi * frequency / static_cast<float>(kSampleRate);
        const float tone = std::sin(phase);
        const float grit = std::sin(phase * 3.73f) * shape.grit;
        samples[static_cast<std::size_t>(index)] =
            (tone + grit) * envelope * shape.gain * volume_;
    }

    if (!SDL_PutAudioStreamData(
            stream_,
            samples.data(),
            static_cast<int>(samples.size() * sizeof(float)))) {
        SDL_LogWarn(SDL_LOG_CATEGORY_AUDIO, "[HAKUI] audio queue failed: %s", SDL_GetError());
    }
}

void HakuiAudio::adjustVolume(float delta) noexcept
{
    volume_ = std::clamp(volume_ + delta, 0.0f, 1.0f);
}

float HakuiAudio::volume() const noexcept
{
    return volume_;
}

void HakuiAudio::shutdown()
{
    if (stream_) {
        SDL_DestroyAudioStream(stream_);
        stream_ = nullptr;
    }
}
