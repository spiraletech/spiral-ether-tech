#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace spiral {

using Clock = std::chrono::steady_clock;
using TimePoint = Clock::time_point;
using Milliseconds = std::chrono::milliseconds;

using SignalId = std::uint64_t;
using CrystalId = std::uint64_t;

using StateValue = std::variant<bool, std::int64_t, double, std::string>;

struct StatePatchEntry {
    std::string key;
    StateValue value;
};

using StatePatch = std::vector<StatePatchEntry>;

// Canonical bus event families recovered from Spiral OS logs.
enum class SignalKind {
    Boot,
    CommandIn,
    CommandOut,
    State,
    Exec,
    Error,
    UI,
    Perf,
    Capability
};

struct Signal {
    SignalId id = 0;
    SignalKind kind = SignalKind::State;
    std::string source;
    std::string destination;
    std::string topic;
    std::string payload;
    TimePoint timestamp = Clock::now();

    // State events may carry a typed merge patch. Entries are applied in order;
    // repeated keys inside one patch therefore resolve last-write-wins.
    StatePatch statePatch;

    // Optional typed wheel target used by Ether Bus transitions.
    std::optional<std::size_t> notch;

    // Steam/pressure rides beside the signal. The bus does not interpret it.
    float pressure = 0.0f;
};

// AUM field canon: A -> U -> M.
enum class AUMPhase {
    A_Emergence,
    U_Sustain,
    M_Return
};

constexpr std::size_t kOctopusNotchCount = 8;
constexpr std::size_t kAUMFieldSize = 7;
constexpr std::size_t kAUMCenter = 3;

} // namespace spiral
