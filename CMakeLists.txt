cmake_minimum_required(VERSION 3.25)

project(Hakui
    VERSION 0.3.0
    DESCRIPTION "Project Hakui native client"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

option(HAKUI_ENABLE_IMVU_CAL3D
    "Enable IMVU's open-source Cal3D skeletal runtime"
    ON
)

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

if(HAKUI_ENABLE_IMVU_CAL3D)
    include(cmake/IMVUCal3D.cmake)
endif()

add_executable(hakui
    src/main.cpp

    src/core/HakuiApp.cpp
    src/core/HakuiApp.hpp

    src/avatar/HakuiSkeleton.cpp
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

target_link_libraries(hakui PRIVATE SDL3::SDL3)

if(HAKUI_ENABLE_IMVU_CAL3D)
    target_link_libraries(hakui PRIVATE hakui_cal3d)
    target_compile_definitions(hakui PRIVATE HAKUI_HAS_IMVU_CAL3D=1)
else()
    target_compile_definitions(hakui PRIVATE HAKUI_HAS_IMVU_CAL3D=0)
endif()

# `src` is Hakui's code. SDL's `test` directory is temporarily exposed only
# so v0.3 can consume SDL's already-precompiled cross-platform cube shaders.
# These bootstrap shaders will be replaced by Hakui-owned shader assets later.
target_include_directories(hakui PRIVATE
    src
    "${sdl3_SOURCE_DIR}/test"
)

if(MSVC)
    target_compile_options(hakui PRIVATE /W4 /permissive-)
else()
    target_compile_options(hakui PRIVATE -Wall -Wextra -Wpedantic)
endif()
