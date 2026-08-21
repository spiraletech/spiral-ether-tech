cmake_minimum_required(VERSION 3.25)

project(Hakui
    VERSION 0.7.0
    DESCRIPTION "Project Hakui native client"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
include(CTest)

# ============================================================
# FIRST-PARTY HAKUI / SPIRAL LAYERS
# ============================================================

# Dependency-free orchestration/plumbing.
include(cmake/SpiralCore.cmake)

# Hakui-owned humanoid hierarchy + attachment-slot metadata.
# No skeletal runtime implementation is allowed inside this target.
include(cmake/HakuiAvatarRig.cmake)

# Hakui-owned world interaction semantics over the Spiral bus/state layer.
include(cmake/HakuiInteraction.cmake)

# Deterministic gameplay rules, independent from SDL and rendering.
include(cmake/HakuiGameplay.cmake)

# Deterministic third-person orbit/framing math, independent from SDL.
include(cmake/HakuiCamera.cmake)

# Weapon-agnostic deterministic combat state and discipline interpretation.
include(cmake/HakuiCombat.cmake)

# Fictional chance games and vendor-neutral in-world terminals.
include(cmake/HakuiTabletop.cmake)

# ============================================================
# BUILD GATES
# ============================================================

option(HAKUI_BUILD_NATIVE_CLIENT
    "Build the SDL3 native Hakui client"
    ON
)

option(HAKUI_ENABLE_IMVU_CAL3D
    "Enable the optional IMVU-Cal3D skeleton crystal backend"
    OFF
)

# ============================================================
# OPTIONAL LEGACY CAPABILITY BACKENDS
# ============================================================

if(HAKUI_ENABLE_IMVU_CAL3D)
    include(cmake/IMVUCal3D.cmake)
endif()

# ============================================================
# SDL3 NATIVE CLIENT — OPTIONAL AT CONFIGURE/BUILD TIME
# ============================================================

if(HAKUI_BUILD_NATIVE_CLIENT)
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

    add_executable(hakui
        src/main.cpp

        src/core/HakuiApp.cpp
        src/core/HakuiApp.hpp

        src/audio/HakuiAudio.cpp
        src/audio/HakuiAudio.hpp

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
            hakui_interaction
            hakui_gameplay
            hakui_camera
            hakui_combat
            hakui_tabletop
    )

    # Deliberate boundary:
    # The Hakui executable NEVER links Cal3D directly. When enabled, the
    # optional `spiral_imvu_cal3d_backend` target exists for CrystalHost/module
    # composition. Legacy runtime dependencies stay out of the client core.

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
endif()
