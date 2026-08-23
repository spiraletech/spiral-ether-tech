#pragma once

#include <filesystem>
#include <string>

struct SDL_Window;

namespace hakui::observer {

// Native presentation boundary for the otherwise platform-neutral observer.
// It reads the already-presented client window and writes one PNG; it has no
// gameplay authority and cannot inject input or mutate simulation state.
bool captureWindowPng(
    SDL_Window* window,
    const std::filesystem::path& destination,
    std::string& error
);

} // namespace hakui::observer
