# G1 native renderer-independent material/lighting/quality contracts.
add_library(hakui_render_style STATIC
    ${CMAKE_CURRENT_LIST_DIR}/../src/render/RenderStyle.cpp
)
target_include_directories(hakui_render_style PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../src)
target_compile_features(hakui_render_style PUBLIC cxx_std_20)
if(MSVC)
    target_compile_options(hakui_render_style PRIVATE /W4 /permissive-)
else()
    target_compile_options(hakui_render_style PRIVATE -Wall -Wextra -Wpedantic)
endif()
if(BUILD_TESTING)
    add_executable(hakui_render_style_spec
        ${CMAKE_CURRENT_LIST_DIR}/../tests/hakui/RenderStyleSpec.cpp)
    target_link_libraries(hakui_render_style_spec PRIVATE hakui_render_style)
    target_compile_features(hakui_render_style_spec PRIVATE cxx_std_20)
    if(MSVC)
        target_compile_options(hakui_render_style_spec PRIVATE /UNDEBUG)
    else()
        target_compile_options(hakui_render_style_spec PRIVATE -UNDEBUG)
    endif()
    add_test(NAME hakui.render_style COMMAND hakui_render_style_spec)
endif()
