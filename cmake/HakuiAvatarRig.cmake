# PROJECT HAKUI :: AVATAR RIG
#
# Hakui's humanoid hierarchy and attachment slots are first-party data.
# This target must stay independent from skeletal runtime implementations.

include(${CMAKE_CURRENT_LIST_DIR}/DependencyFirewall.cmake)

file(GLOB HAKUI_AVATAR_FIREWALL_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/../src/avatar/*.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/../src/avatar/*.cpp"
)

hakui_enforce_first_party_firewall(
    "Hakui Avatar Rig"
    ${HAKUI_AVATAR_FIREWALL_FILES}
)

add_library(hakui_avatar_rig STATIC
    ${CMAKE_CURRENT_LIST_DIR}/../src/avatar/HakuiSkeleton.cpp
)

target_include_directories(hakui_avatar_rig
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/../src
)

target_compile_features(hakui_avatar_rig PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(hakui_avatar_rig PRIVATE /W4 /permissive-)
else()
    target_compile_options(hakui_avatar_rig PRIVATE -Wall -Wextra -Wpedantic)
endif()

option(HAKUI_ENABLE_AVATAR_RIG_SPECS
    "Build the Hakui avatar-rig invariant specification"
    ${BUILD_TESTING}
)

if(HAKUI_ENABLE_AVATAR_RIG_SPECS)
    add_executable(hakui_avatar_rig_spec
        ${CMAKE_CURRENT_LIST_DIR}/../tests/hakui/AvatarRigSpec.cpp
    )

    target_compile_features(hakui_avatar_rig_spec PRIVATE cxx_std_20)
    target_link_libraries(hakui_avatar_rig_spec PRIVATE hakui_avatar_rig)

    if(MSVC)
        target_compile_options(hakui_avatar_rig_spec PRIVATE /W4 /permissive- /UNDEBUG)
    else()
        target_compile_options(hakui_avatar_rig_spec PRIVATE -Wall -Wextra -Wpedantic -UNDEBUG)
    endif()

    if(BUILD_TESTING)
        add_test(NAME hakui.avatar_rig COMMAND hakui_avatar_rig_spec)
    endif()
endif()
