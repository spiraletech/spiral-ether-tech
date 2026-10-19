# M4B: first-party, data-driven avatar shape profiles and rig compatible wardrobe.
# Deliberately no SDL, Cal3D, or renderer dependencies.
add_library(hakui_avatar_derivation STATIC
    ${CMAKE_CURRENT_LIST_DIR}/../src/avatar/derivation/AvatarDerivation.cpp
)
target_include_directories(hakui_avatar_derivation PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../src)
target_compile_features(hakui_avatar_derivation PUBLIC cxx_std_20)
target_link_libraries(hakui_avatar_derivation PUBLIC hakui_glb_import)
if(MSVC)
    target_compile_options(hakui_avatar_derivation PRIVATE /W4 /permissive-)
else()
    target_compile_options(hakui_avatar_derivation PRIVATE -Wall -Wextra -Wpedantic)
endif()
if(BUILD_TESTING)
    add_executable(hakui_avatar_derivation_spec
        ${CMAKE_CURRENT_LIST_DIR}/../tests/hakui/AvatarDerivationSpec.cpp)
    target_link_libraries(hakui_avatar_derivation_spec PRIVATE hakui_avatar_derivation)
    target_compile_features(hakui_avatar_derivation_spec PRIVATE cxx_std_20)
    if(MSVC)
        target_compile_options(hakui_avatar_derivation_spec PRIVATE /UNDEBUG)
    else()
        target_compile_options(hakui_avatar_derivation_spec PRIVATE -UNDEBUG)
    endif()
    add_test(NAME hakui.avatar_derivation COMMAND hakui_avatar_derivation_spec)
endif()
