# PROJECT HAKUI :: AVATAR RIG
#
# Hakui's humanoid hierarchy and attachment slots are first-party data.
# This target must stay independent from skeletal runtime implementations.

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
