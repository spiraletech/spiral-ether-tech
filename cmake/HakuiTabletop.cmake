# PROJECT HAKUI :: TABLETOP + TERMINAL LAYER
#
# Fictional, virtual-credit chance games and vendor-neutral world terminals.
# No real-money, purchase, cash-out, network, or platform APIs belong here.

include(${CMAKE_CURRENT_LIST_DIR}/DependencyFirewall.cmake)

file(GLOB HAKUI_TABLETOP_FIREWALL_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/../src/games/*.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/../src/games/*.cpp"
)

hakui_enforce_first_party_firewall(
    "Hakui Tabletop"
    ${HAKUI_TABLETOP_FIREWALL_FILES}
)

add_library(hakui_tabletop STATIC
    ${CMAKE_CURRENT_LIST_DIR}/../src/games/CardDeck.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../src/games/Dice.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../src/games/BlackjackTable.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../src/games/GameTerminal.cpp
)

target_include_directories(hakui_tabletop
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/../src
)

target_compile_features(hakui_tabletop PUBLIC cxx_std_20)
target_link_libraries(hakui_tabletop PUBLIC hakui_interaction)

if(MSVC)
    target_compile_options(hakui_tabletop PRIVATE /W4 /permissive-)
else()
    target_compile_options(hakui_tabletop PRIVATE -Wall -Wextra -Wpedantic)
endif()

option(HAKUI_ENABLE_TABLETOP_SPECS
    "Build the Hakui tabletop and terminal specifications"
    ${BUILD_TESTING}
)

if(HAKUI_ENABLE_TABLETOP_SPECS)
    add_executable(hakui_tabletop_spec
        ${CMAKE_CURRENT_LIST_DIR}/../tests/hakui/TabletopSpec.cpp
    )

    target_compile_features(hakui_tabletop_spec PRIVATE cxx_std_20)
    target_link_libraries(hakui_tabletop_spec PRIVATE hakui_tabletop)

    if(MSVC)
        target_compile_options(hakui_tabletop_spec PRIVATE /W4 /permissive- /UNDEBUG)
    else()
        target_compile_options(hakui_tabletop_spec PRIVATE -Wall -Wextra -Wpedantic -UNDEBUG)
    endif()

    if(BUILD_TESTING)
        add_test(NAME hakui.tabletop COMMAND hakui_tabletop_spec)
    endif()
endif()
