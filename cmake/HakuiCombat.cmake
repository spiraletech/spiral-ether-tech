# PROJECT HAKUI :: COMBAT LAYER
#
# Deterministic, weapon-agnostic encounter rules. Discipline profiles interpret
# intent; SDL input, animation, audio, and rendering stay downstream.

include(${CMAKE_CURRENT_LIST_DIR}/DependencyFirewall.cmake)

file(GLOB HAKUI_COMBAT_FIREWALL_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/../src/combat/*.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/../src/combat/*.cpp"
)

hakui_enforce_first_party_firewall(
    "Hakui Combat"
    ${HAKUI_COMBAT_FIREWALL_FILES}
)

add_library(hakui_combat STATIC
    ${CMAKE_CURRENT_LIST_DIR}/../src/combat/CombatSimulation.cpp
)

target_include_directories(hakui_combat
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/../src
)

target_compile_features(hakui_combat PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(hakui_combat PRIVATE /W4 /permissive-)
else()
    target_compile_options(hakui_combat PRIVATE -Wall -Wextra -Wpedantic)
endif()

option(HAKUI_ENABLE_COMBAT_SPECS
    "Build the Hakui deterministic-combat specification"
    ${BUILD_TESTING}
)

if(HAKUI_ENABLE_COMBAT_SPECS)
    add_executable(hakui_combat_spec
        ${CMAKE_CURRENT_LIST_DIR}/../tests/hakui/CombatSpec.cpp
    )

    target_compile_features(hakui_combat_spec PRIVATE cxx_std_20)
    target_link_libraries(hakui_combat_spec PRIVATE hakui_combat)

    if(MSVC)
        target_compile_options(hakui_combat_spec PRIVATE /W4 /permissive- /UNDEBUG)
    else()
        target_compile_options(hakui_combat_spec PRIVATE -Wall -Wextra -Wpedantic -UNDEBUG)
    endif()

    if(BUILD_TESTING)
        add_test(NAME hakui.combat COMMAND hakui_combat_spec)
    endif()
endif()
