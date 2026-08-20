#pragma once

#include <functional>

#include "spiral/crystal/Crystal.hpp"

namespace spiral {

// IMVU-Cal3D is intentionally represented as a capability crystal.
// This header has zero Cal3D includes and therefore cannot make Hakui core
// depend on Boost/RapidXML/Cal3D at compile time.
class ImvuSkeletonCrystal final : public Crystal {
public:
    static constexpr CrystalId kId = 0x494D5655ULL; // 'IMVU'

    struct BackendHooks {
        std::function<bool()> available;
        std::function<bool()> boot;
        std::function<void(float)> tick;
        std::function<void()> shutdown;
        std::function<void(const Signal&)> onSignal;
    };

    explicit ImvuSkeletonCrystal(BackendHooks hooks = {});

    CrystalId id() const noexcept override;
    std::string_view name() const noexcept override;

    bool emerge(RouterBus& bus) override;
    void sustain(float dtSeconds) override;
    void returnToDormant() override;
    void onSignal(const Signal& signal) override;

    State state() const noexcept override;
    bool healthy() const noexcept override;

private:
    BackendHooks hooks_;
    State state_ = State::Dormant;
};

} // namespace spiral
