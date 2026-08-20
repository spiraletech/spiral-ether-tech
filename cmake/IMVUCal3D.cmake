# PROJECT HAKUI :: IMVU CAL3D SKELETON CRYSTAL BACKEND
#
# IMVU public fork: https://github.com/imvu/cal3d
# License: LGPL-2.1-or-later
# Pinned commit: 8522b45cdc26960f779323cc9a0ef50678eda7de
#
# IMPORTANT:
# This is intentionally NOT a full Cal3D build. Hakui only needs the skeletal
# transform runtime at this stage. Loader/XML/saver/model/renderer sources are
# excluded so RapidXML and unrelated legacy dependencies cannot enter the
# Hakui/Spiral core graph.

include(FetchContent)

FetchContent_Declare(
    IMVUCal3D
    GIT_REPOSITORY https://github.com/imvu/cal3d.git
    GIT_TAG 8522b45cdc26960f779323cc9a0ef50678eda7de
    GIT_SHALLOW FALSE
)

FetchContent_GetProperties(IMVUCal3D)
if(NOT imvucal3d_POPULATED)
    FetchContent_Populate(IMVUCal3D)
endif()

set(HAKUI_CAL3D_SKELETON_SOURCES
    "${imvucal3d_SOURCE_DIR}/src/cal3d/bone.cpp"
    "${imvucal3d_SOURCE_DIR}/src/cal3d/bonetransform.cpp"
    "${imvucal3d_SOURCE_DIR}/src/cal3d/corebone.cpp"
    "${imvucal3d_SOURCE_DIR}/src/cal3d/coreskeleton.cpp"
    "${imvucal3d_SOURCE_DIR}/src/cal3d/error.cpp"
    "${imvucal3d_SOURCE_DIR}/src/cal3d/matrix.cpp"
    "${imvucal3d_SOURCE_DIR}/src/cal3d/memory.cpp"
    "${imvucal3d_SOURCE_DIR}/src/cal3d/platform.cpp"
    "${imvucal3d_SOURCE_DIR}/src/cal3d/quaternion.cpp"
    "${imvucal3d_SOURCE_DIR}/src/cal3d/skeleton.cpp"
    "${imvucal3d_SOURCE_DIR}/src/cal3d/vector.cpp"
)

add_library(hakui_cal3d_skeleton SHARED ${HAKUI_CAL3D_SKELETON_SOURCES})

set_target_properties(hakui_cal3d_skeleton PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    OUTPUT_NAME "hakui_cal3d_skeleton"
)

target_include_directories(hakui_cal3d_skeleton
    PUBLIC
        "${imvucal3d_SOURCE_DIR}/src"
)

# IMVU's fork already contains an explicit std::shared_ptr/std::optional path.
# Use it so the skeletal slice does not require Boost.
target_compile_definitions(hakui_cal3d_skeleton
    PUBLIC
        USE_CAL3D_WITH_CPP_11=1
    PRIVATE
        CAL3D_EXPORTS
)

if(MSVC)
    target_compile_definitions(hakui_cal3d_skeleton PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(hakui_cal3d_skeleton PRIVATE /W2 /wd4244 /wd4267 /wd4996)
else()
    target_compile_options(hakui_cal3d_skeleton PRIVATE -Wno-deprecated-declarations -Wno-unused-parameter)
endif()

# Crystal adapter/module: these are the ONLY Hakui-owned translation units
# allowed to depend on the optional Cal3D skeletal target. Public headers remain
# free of Cal3D types; legacy runtime objects are hidden inside backend PIMPL.
add_library(spiral_imvu_cal3d_backend STATIC
    "${CMAKE_CURRENT_LIST_DIR}/../src/spiral/crystal/backend/ImvuCal3DBackend.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../src/spiral/crystal/backend/ImvuCal3DModule.cpp"
)

target_include_directories(spiral_imvu_cal3d_backend
    PUBLIC
        "${CMAKE_CURRENT_LIST_DIR}/../src"
)

target_compile_features(spiral_imvu_cal3d_backend PUBLIC cxx_std_20)

target_link_libraries(spiral_imvu_cal3d_backend
    PUBLIC
        spiral_core
        hakui_avatar_rig
    PRIVATE
        hakui_cal3d_skeleton
)

option(HAKUI_ENABLE_IMVU_BACKEND_SPECS
    "Build the isolated IMVU-Cal3D crystal backend invariant specification"
    ${BUILD_TESTING}
)

if(HAKUI_ENABLE_IMVU_BACKEND_SPECS)
    add_executable(imvu_cal3d_backend_spec
        "${CMAKE_CURRENT_LIST_DIR}/../tests/spiral/ImvuCal3DBackendSpec.cpp"
    )

    target_compile_features(imvu_cal3d_backend_spec PRIVATE cxx_std_20)
    target_link_libraries(imvu_cal3d_backend_spec
        PRIVATE
            spiral_imvu_cal3d_backend
    )

    if(MSVC)
        target_compile_options(imvu_cal3d_backend_spec PRIVATE /W4 /permissive- /UNDEBUG)
    else()
        target_compile_options(imvu_cal3d_backend_spec PRIVATE -Wall -Wextra -Wpedantic -UNDEBUG)
    endif()

    if(BUILD_TESTING)
        add_test(NAME spiral.imvu_cal3d_backend COMMAND imvu_cal3d_backend_spec)
    endif()
endif()
