cmake_minimum_required(VERSION 3.25)

project(Hakui
    VERSION 0.4.0
    DESCRIPTION "Project Hakui native client"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# ============================================================
# FIRST-PARTY HAKUI / SPIRAL LAYERS
# ============================================================

# Dependency-free orchestration/plumbing.
include(cmake/SpiralCore.cmake)

# Hakui-owned humanoid hierarchy + attachment-slot metadata.
# No skeletal runtime implementation is allowed inside this target.
include(cmake/HakuiAvatarRig.cmake)

# ============================================================
# OPTIONAL LEGACY CAPABILITY BACKENDS
# ============================================================

option(HAKUI_ENABLE_IMVU_CAL3D
    "Enable the optional IMVU-Cal3D skeleton crystal backend"
    OFF
)

if(HAKUI_ENABLE_IMVU_CAL3D)
    include(cmake/IMVUCal3D.cmake)
endif()

# ============================================================
# SDL3 PLATFORM / RENDERING LAYER
# ============================================================

set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.4.14
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(SDL3)

# ============================================================
# HAKUI CLIENT
# ============================================================

add_executable(hakui
    src/main.cpp

    src/core/HakuiApp.cpp
    src/core/HakuiApp.hpp

    # Headers are listed for IDE visibility; implementation lives in
    # hakui_avatar_rig and is not compiled twice here.
    src/avatar/HakuiSkeleton.hpp
    src/avatar/AvatarAttachment.hpp

    src/render/DebugWorldRenderer.cpp
    src/render/DebugWorldRenderer.hpp
    src/render/Math3D.hpp

    src/player/PlayerState.hpp
    src/world/WorldState.hpp

    src/systems/LocomotionRouter.hpp
    src/systems/Interactable.hpp
)

target_link_libraries(hakui
    PRIVATE
        SDL3::SDL3
        spiral_core
        hakui_avatar_rig
)

# Deliberate boundary:
# The Hakui executable NEVER links Cal3D directly. When enabled, the optional
# `spiral_imvu_cal3d_backend` target exists for a crystal host/composition layer
# to instantiate. This keeps legacy runtime dependencies out of the client core.

# `src` is Hakui's code. SDL's `test` directory is temporarily exposed only
# so the debug renderer can consume SDL's precompiled bootstrap cube shaders.
# These will be replaced with Hakui-owned shader assets later.
target_include_directories(hakui PRIVATE
    src
    "${sdl3_SOURCE_DIR}/test"
)

if(MSVC)
    target_compile_options(hakui PRIVATE /W4 /permissive-)
else()
    target_compile_options(hakui PRIVATE -Wall -Wextra -Wpedantic)
endif()
