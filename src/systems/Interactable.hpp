#pragma once

#include <string_view>

class PlayerState;

class Interactable {
public:
    virtual ~Interactable() = default;
    virtual std::string_view prompt() const = 0;
    virtual void interact(PlayerState& player) = 0;
};
