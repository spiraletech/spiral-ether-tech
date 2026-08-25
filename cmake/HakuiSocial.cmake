# PROJECT HAKUI :: SOCIAL LANGUAGE LAYER
#
# Deterministic local chat history, speech intent, bubble lifetime, and social
# gesture selection. Platform events and rendering remain downstream.

include(${CMAKE_CURRENT_LIST_DIR}/DependencyFirewall.cmake)

hakui_enforce_first_party_firewall(
    "Hakui Social"
    ${CMAKE_CURRENT_LIST_DIR}/../src/social/ChatSystem.hpp
    ${CMAKE_CURRENT_LIST_DIR}/../src/social/ChatSystem.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../src/social/ChatBubblePresentation.hpp
    ${CMAKE_CURRENT_LIST_DIR}/../src/social/ChatBubblePresentation.cpp
)

add_library(hakui_social STATIC
    ${CMAKE_CURRENT_LIST_DIR}/../src/social/ChatSystem.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../src/social/ChatBubblePresentation.cpp
)

target_include_directories(hakui_social
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/../src
)

target_compile_features(hakui_social PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(hakui_social PRIVATE /W4 /permissive-)
else()
    target_compile_options(hakui_social PRIVATE -Wall -Wextra -Wpedantic)
endif()

option(HAKUI_ENABLE_SOCIAL_SPECS
    "Build the Hakui deterministic social-language specification"
    ${BUILD_TESTING}
)

if(HAKUI_ENABLE_SOCIAL_SPECS)
    add_executable(hakui_social_spec
        ${CMAKE_CURRENT_LIST_DIR}/../tests/hakui/ChatSystemSpec.cpp
    )
    target_compile_features(hakui_social_spec PRIVATE cxx_std_20)
    target_link_libraries(hakui_social_spec PRIVATE hakui_social)
    if(MSVC)
        target_compile_options(hakui_social_spec PRIVATE /W4 /permissive- /UNDEBUG)
    else()
        target_compile_options(hakui_social_spec PRIVATE -Wall -Wextra -Wpedantic -UNDEBUG)
    endif()
    if(BUILD_TESTING)
        add_test(NAME hakui.social COMMAND hakui_social_spec)
    endif()
endif()
