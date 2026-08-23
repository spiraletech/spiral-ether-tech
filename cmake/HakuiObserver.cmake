# PROJECT HAKUI :: EXPERT OBSERVER
#
# Deterministic, read-only snapshot schemas and exporters. Native window/frame
# capture remains in the SDL executable boundary.

include(${CMAKE_CURRENT_LIST_DIR}/DependencyFirewall.cmake)

hakui_enforce_first_party_firewall(
    "Hakui Expert Observer"
    ${CMAKE_CURRENT_LIST_DIR}/../src/observer/ExpertObserver.hpp
    ${CMAKE_CURRENT_LIST_DIR}/../src/observer/ExpertObserver.cpp
)

add_library(hakui_observer STATIC
    ${CMAKE_CURRENT_LIST_DIR}/../src/observer/ExpertObserver.cpp
)

target_include_directories(hakui_observer
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/../src
)

target_compile_features(hakui_observer PUBLIC cxx_std_20)
target_link_libraries(hakui_observer PUBLIC hakui_input)

if(MSVC)
    target_compile_options(hakui_observer PRIVATE /W4 /permissive-)
else()
    target_compile_options(hakui_observer PRIVATE -Wall -Wextra -Wpedantic)
endif()

option(HAKUI_ENABLE_OBSERVER_SPECS
    "Build the HAKUI read-only observer specification"
    ${BUILD_TESTING}
)

if(HAKUI_ENABLE_OBSERVER_SPECS)
    add_executable(hakui_observer_spec
        ${CMAKE_CURRENT_LIST_DIR}/../tests/hakui/ObserverSpec.cpp
    )
    target_compile_features(hakui_observer_spec PRIVATE cxx_std_20)
    target_link_libraries(hakui_observer_spec PRIVATE hakui_observer)
    if(MSVC)
        target_compile_options(hakui_observer_spec PRIVATE /W4 /permissive- /UNDEBUG)
    else()
        target_compile_options(hakui_observer_spec PRIVATE -Wall -Wextra -Wpedantic -UNDEBUG)
    endif()
    if(BUILD_TESTING)
        add_test(NAME hakui.observer COMMAND hakui_observer_spec)
    endif()
endif()
