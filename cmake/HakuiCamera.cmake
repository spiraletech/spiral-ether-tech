# PROJECT HAKUI :: THIRD-PERSON CAMERA MODEL
#
# Deterministic orbit/framing state. Native mouse capture and SDL rendering are
# adapters around this target, never part of its math contract.

include(${CMAKE_CURRENT_LIST_DIR}/DependencyFirewall.cmake)

file(GLOB HAKUI_CAMERA_FIREWALL_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/../src/camera/*.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/../src/camera/*.cpp"
)

hakui_enforce_first_party_firewall(
    "Hakui Camera"
    ${HAKUI_CAMERA_FIREWALL_FILES}
)

add_library(hakui_camera STATIC
    ${CMAKE_CURRENT_LIST_DIR}/../src/camera/ThirdPersonCameraRig.cpp
)

target_include_directories(hakui_camera PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../src)
target_compile_features(hakui_camera PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(hakui_camera PRIVATE /W4 /permissive-)
else()
    target_compile_options(hakui_camera PRIVATE -Wall -Wextra -Wpedantic)
endif()

option(HAKUI_ENABLE_CAMERA_SPECS
    "Build the Hakui deterministic third-person-camera specification"
    ${BUILD_TESTING}
)

if(HAKUI_ENABLE_CAMERA_SPECS)
    add_executable(hakui_camera_spec
        ${CMAKE_CURRENT_LIST_DIR}/../tests/hakui/CameraSpec.cpp
    )
    target_compile_features(hakui_camera_spec PRIVATE cxx_std_20)
    target_link_libraries(hakui_camera_spec PRIVATE hakui_camera)

    if(MSVC)
        target_compile_options(hakui_camera_spec PRIVATE /W4 /permissive- /UNDEBUG)
    else()
        target_compile_options(hakui_camera_spec PRIVATE -Wall -Wextra -Wpedantic -UNDEBUG)
    endif()

    if(BUILD_TESTING)
        add_test(NAME hakui.camera COMMAND hakui_camera_spec)
    endif()
endif()
