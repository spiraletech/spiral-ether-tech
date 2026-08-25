cmake_minimum_required(VERSION 3.25)

project(Hakui
    VERSION 0.861.0
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

include(cmake/SpiralCore.cmake)
include(cmake/HakuiAvatarRig.cmake)
include(cmake/HakuiInteraction.cmake)
include(cmake/HakuiGameplay.cmake)
include(cmake/HakuiInput.cmake)
include(cmake/HakuiSocial.cmake)
include(cmake/HakuiObserver.cmake)
include(cmake/HakuiCamera.cmake)
include(cmake/HakuiCombat.cmake)
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

if(HAKUI_ENABLE_IMVU_CAL3D)
    include(cmake/IMVUCal3D.cmake)
endif()

# ============================================================
# SDL3 NATIVE CLIENTS
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

    execute_process(
        COMMAND git -c safe.directory=${CMAKE_CURRENT_SOURCE_DIR} rev-parse --short=12 HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE HAKUI_GIT_SHA
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    execute_process(
        COMMAND git -c safe.directory=${CMAKE_CURRENT_SOURCE_DIR} branch --show-current
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE HAKUI_GIT_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(NOT HAKUI_GIT_SHA)
        set(HAKUI_GIT_SHA "unknown")
    endif()
    if(NOT HAKUI_GIT_BRANCH)
        set(HAKUI_GIT_BRANCH "detached-or-unknown")
    endif()
    string(TIMESTAMP HAKUI_BUILD_TIMESTAMP "%Y-%m-%dT%H:%M:%SZ" UTC)
    if(CMAKE_BUILD_TYPE)
        set(HAKUI_BUILD_CONFIGURATION ${CMAKE_BUILD_TYPE})
    else()
        set(HAKUI_BUILD_CONFIGURATION "default")
    endif()

    add_executable(hakui
        src/main.cpp
        src/core/HakuiApp.cpp
        src/core/HakuiApp.hpp
        src/observer/NativeFrameCapture.cpp
        src/observer/NativeFrameCapture.hpp
        src/input/SdlInputBridge.cpp
        src/input/SdlInputBridge.hpp
        src/audio/HakuiAudio.cpp
        src/audio/HakuiAudio.hpp
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
            hakui_input
            hakui_social
            hakui_observer
            hakui_camera
            hakui_combat
            hakui_tabletop
    )

    target_include_directories(hakui PRIVATE
        src
        "${sdl3_SOURCE_DIR}/test"
    )

    target_compile_definitions(hakui PRIVATE
        HAKUI_GIT_SHA="${HAKUI_GIT_SHA}"
        HAKUI_GIT_BRANCH="${HAKUI_GIT_BRANCH}"
        HAKUI_BUILD_TIMESTAMP="${HAKUI_BUILD_TIMESTAMP}"
        HAKUI_BUILD_CONFIGURATION="${HAKUI_BUILD_CONFIGURATION}"
    )

    # ------------------------------------------------------------
    # HAKUI MANNEQUIN LAB
    # ------------------------------------------------------------
    # This is deliberately a separate executable. It reuses Hakui's low-level
    # render/rig math, but does not boot HakuiApp, world simulation, chat,
    # combat, casino, ride input, or Spiral orchestration. It exists to study
    # human proportions, pose targets, joints and future skinning in isolation.
    add_executable(hakui_mannequin_lab
        src/mannequin_lab/main.cpp
        src/mannequin_lab/MannequinLabApp.cpp
        src/mannequin_lab/MannequinLabApp.hpp
        src/render/DebugWorldRenderer.cpp
        src/render/DebugWorldRenderer.hpp
        src/render/Math3D.hpp
        src/player/PlayerState.hpp
        src/world/WorldGeometry.hpp
    )

    set_target_properties(hakui_mannequin_lab PROPERTIES
        OUTPUT_NAME "HAKUI-MANNEQUIN-LAB"
    )

    target_link_libraries(hakui_mannequin_lab
        PRIVATE
            SDL3::SDL3
            hakui_avatar_rig
            hakui_gameplay
            hakui_social
            hakui_camera
            hakui_combat
    )

    target_include_directories(hakui_mannequin_lab PRIVATE
        src
        "${sdl3_SOURCE_DIR}/test"
    )

    if(WIN32)
        target_link_libraries(hakui PRIVATE gdiplus)
        target_link_libraries(hakui_mannequin_lab PRIVATE gdiplus)
    endif()

    if(MSVC)
        target_compile_options(hakui PRIVATE /W4 /permissive-)
        target_compile_options(hakui_mannequin_lab PRIVATE /W4 /permissive-)
    else()
        target_compile_options(hakui PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(hakui_mannequin_lab PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()
