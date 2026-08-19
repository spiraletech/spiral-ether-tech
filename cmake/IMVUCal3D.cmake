# PROJECT HAKUI :: IMVU CAL3D BRIDGE
# IMVU public fork: https://github.com/imvu/cal3d
# License: LGPL-2.1-or-later
# Pinned commit: 8522b45cdc26960f779323cc9a0ef50678eda7de

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

file(GLOB CAL3D_SOURCES CONFIGURE_DEPENDS
    "${imvucal3d_SOURCE_DIR}/src/cal3d/*.cpp"
)
list(FILTER CAL3D_SOURCES EXCLUDE REGEX ".*/PCH\\.cpp$")

add_library(hakui_cal3d SHARED ${CAL3D_SOURCES})
set_target_properties(hakui_cal3d PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    OUTPUT_NAME "hakui_cal3d"
)

target_include_directories(hakui_cal3d PUBLIC "${imvucal3d_SOURCE_DIR}/src")
target_compile_definitions(hakui_cal3d PRIVATE CAL3D_EXPORTS)

if(MSVC)
    target_compile_options(hakui_cal3d PRIVATE /W2 /wd4244 /wd4267 /wd4996)
else()
    target_compile_options(hakui_cal3d PRIVATE -Wno-deprecated-declarations -Wno-unused-parameter)
endif()
