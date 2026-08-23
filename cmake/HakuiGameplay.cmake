# PROJECT HAKUI :: GAMEPLAY LAYER
#
# Deterministic first-party gameplay rules. This target owns simulation logic,
# not platform input, rendering, audio, or persistence.

include(${CMAKE_CURRENT_LIST_DIR}/DependencyFirewall.cmake)

file(GLOB HAKUI_GAMEPLAY_FIREWALL_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/../src/player/*.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/../src/player/*.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../src/world/*.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/../src/world/*.cpp"
)

hakui_enforce_first_party_firewall(
    "Hakui Gameplay"
    ${HAKUI_GAMEPLAY_FIREWALL_FILES}
)

add_library(hakui_gameplay STATIC
    ${CMAKE_CURRENT_LIST_DIR}/../src/player/PlayerMovementController.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../src/player/RideableMovementController.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../src/world/BlackRoom.cpp
)

target_include_directories(hakui_gameplay
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/../src
)

target_compile_features(hakui_gameplay PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(hakui_gameplay PRIVATE /W4 /permissive-)
else()
    target_compile_options(hakui_gameplay PRIVATE -Wall -Wextra -Wpedantic)
endif()

option(HAKUI_ENABLE_GAMEPLAY_SPECS
    "Build the Hakui deterministic-gameplay specification"
    ${BUILD_TESTING}
)

if(HAKUI_ENABLE_GAMEPLAY_SPECS)
    add_executable(hakui_gameplay_spec
        ${CMAKE_CURRENT_LIST_DIR}/../tests/hakui/GameplayMovementSpec.cpp
    )

    add_executable(hakui_rideable_spec
        ${CMAKE_CURRENT_LIST_DIR}/../tests/hakui/RideableMovementSpec.cpp
    )

    target_compile_features(hakui_gameplay_spec PRIVATE cxx_std_20)
    target_link_libraries(hakui_gameplay_spec PRIVATE hakui_gameplay)
    target_compile_features(hakui_rideable_spec PRIVATE cxx_std_20)
    target_link_libraries(hakui_rideable_spec PRIVATE hakui_gameplay)

    if(MSVC)
        target_compile_options(hakui_gameplay_spec PRIVATE /W4 /permissive- /UNDEBUG)
        target_compile_options(hakui_rideable_spec PRIVATE /W4 /permissive- /UNDEBUG)
    else()
        target_compile_options(hakui_gameplay_spec PRIVATE -Wall -Wextra -Wpedantic -UNDEBUG)
        target_compile_options(hakui_rideable_spec PRIVATE -Wall -Wextra -Wpedantic -UNDEBUG)
    endif()

    if(BUILD_TESTING)
        add_test(NAME hakui.gameplay_movement COMMAND hakui_gameplay_spec)
        add_test(NAME hakui.rideable_movement COMMAND hakui_rideable_spec)
    endif()
endif()
