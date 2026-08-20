# PROJECT HAKUI :: SPIRAL CORE
#
# Dependency rule:
#   Spiral Core must remain plain C++20 and must not depend on SDL, Cal3D,
#   Boost, RapidXML, rendering, audio, or platform APIs.
#
# Third-party capability backends attach outside this target through crystals.

add_library(spiral_core STATIC
    ${CMAKE_CURRENT_LIST_DIR}/../src/spiral/SpiralKernel.cpp

    ${CMAKE_CURRENT_LIST_DIR}/../src/spiral/aum/AUMField.cpp

    ${CMAKE_CURRENT_LIST_DIR}/../src/spiral/bus/EtherBus.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../src/spiral/bus/RouterBus.cpp

    ${CMAKE_CURRENT_LIST_DIR}/../src/spiral/crystal/CrystalGrid.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../src/spiral/crystal/CrystalHost.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../src/spiral/crystal/ImvuSkeletonCrystal.cpp

    ${CMAKE_CURRENT_LIST_DIR}/../src/spiral/engine/SteamEngine.cpp

    ${CMAKE_CURRENT_LIST_DIR}/../src/spiral/ledger/MonolithLedger.cpp
)

target_include_directories(spiral_core
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/../src
)

target_compile_features(spiral_core PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(spiral_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(spiral_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

option(HAKUI_ENABLE_SPIRAL_LOGIC_SPECS
    "Build the source-level Spiral engine invariant specification executable"
    OFF
)

if(HAKUI_ENABLE_SPIRAL_LOGIC_SPECS)
    add_executable(spiral_logic_spec
        ${CMAKE_CURRENT_LIST_DIR}/../tests/spiral/SpiralLogicSpec.cpp
    )

    target_link_libraries(spiral_logic_spec PRIVATE spiral_core)
    target_compile_features(spiral_logic_spec PRIVATE cxx_std_20)
endif()
