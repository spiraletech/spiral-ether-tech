# PROJECT HAKUI :: SEMANTIC INPUT
#
# Platform-neutral action, axis, prompt, and device-activity resolution. SDL
# enters only through the native SdlInputBridge and is forbidden from this
# deterministic target.

include(${CMAKE_CURRENT_LIST_DIR}/DependencyFirewall.cmake)

hakui_enforce_first_party_firewall(
    "Hakui Input"
    ${CMAKE_CURRENT_LIST_DIR}/../src/input/HakuiInput.hpp
    ${CMAKE_CURRENT_LIST_DIR}/../src/input/HakuiInput.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../src/input/RideControlInterpreter.hpp
    ${CMAKE_CURRENT_LIST_DIR}/../src/input/RideControlInterpreter.cpp
)

add_library(hakui_input STATIC
    ${CMAKE_CURRENT_LIST_DIR}/../src/input/HakuiInput.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../src/input/RideControlInterpreter.cpp
)

target_include_directories(hakui_input
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/../src
)

target_compile_features(hakui_input PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(hakui_input PRIVATE /W4 /permissive-)
else()
    target_compile_options(hakui_input PRIVATE -Wall -Wextra -Wpedantic)
endif()

option(HAKUI_ENABLE_INPUT_SPECS
    "Build the Hakui semantic-input specification"
    ${BUILD_TESTING}
)

if(HAKUI_ENABLE_INPUT_SPECS)
    add_executable(hakui_input_spec
        ${CMAKE_CURRENT_LIST_DIR}/../tests/hakui/InputSpec.cpp
    )
    target_compile_features(hakui_input_spec PRIVATE cxx_std_20)
    target_link_libraries(hakui_input_spec PRIVATE hakui_input)
    if(MSVC)
        target_compile_options(hakui_input_spec PRIVATE /W4 /permissive- /UNDEBUG)
    else()
        target_compile_options(hakui_input_spec PRIVATE -Wall -Wextra -Wpedantic -UNDEBUG)
    endif()
    if(BUILD_TESTING)
        add_test(NAME hakui.input COMMAND hakui_input_spec)
    endif()
endif()
