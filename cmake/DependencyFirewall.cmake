# PROJECT HAKUI :: FIRST-PARTY DEPENDENCY FIREWALL
#
# Usage:
#   hakui_enforce_first_party_firewall("Spiral Core" file1 file2 ...)
#
# This is a source-boundary check, not a package manager. It prevents accidental
# direct includes of platform/legacy dependencies inside first-party core code.

function(hakui_enforce_first_party_firewall label)
    set(forbidden_markers
        "#include <SDL3/"
        "#include \"SDL3/"
        "#include <cal3d/"
        "#include \"cal3d/"
        "#include <boost/"
        "#include \"boost/"
        "#include <rapidxml"
        "#include \"rapidxml"
    )

    foreach(source_file ${ARGN})
        if(NOT EXISTS "${source_file}")
            continue()
        endif()

        file(READ "${source_file}" source_text)

        foreach(marker IN LISTS forbidden_markers)
            string(FIND "${source_text}" "${marker}" marker_pos)
            if(NOT marker_pos EQUAL -1)
                message(FATAL_ERROR
                    "${label} dependency firewall violation:\n"
                    "  file: ${source_file}\n"
                    "  forbidden include marker: ${marker}\n"
                    "Move third-party runtime integration behind a crystal/backend boundary."
                )
            endif()
        endforeach()
    endforeach()
endfunction()
