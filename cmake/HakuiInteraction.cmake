# PROJECT HAKUI :: INTERACTION LAYER
#
# First-party world interaction semantics. Depends on Spiral Core for typed bus
# events/state patches, but remains independent from SDL, Cal3D, rendering, and
# platform APIs.

include(${CMAKE_CURRENT_LIST_DIR}/DependencyFirewall.cmake)

file(GLOB HAKUI_INTERACTION_FIREWALL_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/../src/interaction/*.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/../src/interaction/*.cpp"
)

hakui_enforce_first_party_firewall(
    "Hakui Interaction"
    ${HAKUI_INTERACTION_FIREWALL_FILES}
)

add_library(hakui_interaction STATIC
    ${CMAKE_CURRENT_LIST_DIR}/../src/interaction/InteractionService.cpp
)

target_include_directories(hakui_interaction
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/../src
)

target_compile_features(hakui_interaction PUBLIC cxx_std_20)
target_link_libraries(hakui_interaction PUBLIC spiral_core)

if(MSVC)
    target_compile_options(hakui_interaction PRIVATE /W4 /permissive-)
else()
    target_compile_options(hakui_interaction PRIVATE -Wall -Wextra -Wpedantic)
endif()

option(HAKUI_ENABLE_INTERACTION_SPECS
    "Build the Hakui world-interaction invariant specification"
    ${BUILD_TESTING}
)

if(HAKUI_ENABLE_INTERACTION_SPECS)
    add_executable(hakui_interaction_spec
        ${CMAKE_CURRENT_LIST_DIR}/../tests/hakui/InteractionSpec.cpp
    )

    target_compile_features(hakui_interaction_spec PRIVATE cxx_std_20)
    target_link_libraries(hakui_interaction_spec PRIVATE hakui_interaction)

    if(MSVC)
        target_compile_options(hakui_interaction_spec PRIVATE /W4 /permissive- /UNDEBUG)
    else()
        target_compile_options(hakui_interaction_spec PRIVATE -Wall -Wextra -Wpedantic -UNDEBUG)
    endif()

    if(BUILD_TESTING)
        add_test(NAME hakui.interaction COMMAND hakui_interaction_spec)
    endif()
endif()
