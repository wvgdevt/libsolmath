cmake_minimum_required(VERSION 3.16)

# Run like:
# cmake -DROOT_DIR=/path/to/repo
#       -DTEMPLATE=/path/to/template.txt
#       -DOWNER="Wins Vega"
#       -DYEAR=2026
#       -P cmake/update_license.cmake

if (NOT DEFINED ROOT_DIR)
    get_filename_component(ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
endif ()

if (NOT DEFINED TEMPLATE)
    set(TEMPLATE "${ROOT_DIR}/cmake/license/template.txt")
endif ()

if (NOT DEFINED OWNER)
    set(OWNER "Unknown")
endif ()

if (NOT DEFINED YEAR)
    string(TIMESTAMP YEAR "%Y")
endif ()

if (NOT EXISTS "${TEMPLATE}")
    message(FATAL_ERROR "Template not found: ${TEMPLATE}")
endif ()

# Read and expand template
file(READ "${TEMPLATE}" LICENSE_TEMPLATE_RAW)

# Normalize template line endings
string(REPLACE "\r\n" "\n" LICENSE_BLOCK "${LICENSE_TEMPLATE_RAW}")
string(REPLACE "\r" "\n" LICENSE_BLOCK "${LICENSE_BLOCK}")

string(REPLACE "@OWNER@" "${OWNER}" LICENSE_BLOCK "${LICENSE_BLOCK}")
string(REPLACE "@YEAR@" "${YEAR}" LICENSE_BLOCK "${LICENSE_BLOCK}")

# Ensure it ends with exactly one blank line after the block
if (NOT LICENSE_BLOCK MATCHES "\n$")
    string(APPEND LICENSE_BLOCK "\n")
endif ()
string(APPEND LICENSE_BLOCK "\n")

# Collect files
file(GLOB_RECURSE SOURCE_FILES
        RELATIVE "${ROOT_DIR}"
        "${ROOT_DIR}/*.h" "${ROOT_DIR}/*.hpp" "${ROOT_DIR}/*.hh"
        "${ROOT_DIR}/*.c" "${ROOT_DIR}/*.cc" "${ROOT_DIR}/*.cpp" "${ROOT_DIR}/*.cxx"
)

# Simple excludes (edit as you like)
set(EXCLUDE_SUBSTRINGS
        "/build/"
        "/.git/"
        "/third_party/"
        "/external/"
        "/_deps/"
)

set(FILES "")
foreach (f IN LISTS SOURCE_FILES)
    set(skip FALSE)
    foreach (ex IN LISTS EXCLUDE_SUBSTRINGS)
        if (f MATCHES ".*${ex}.*")
            set(skip TRUE)
        endif ()
    endforeach ()
    if (NOT skip)
        list(APPEND FILES "${f}")
    endif ()
endforeach ()

# Markers we search for in the source files
set(BEGIN_MARK "/* BEGIN_LICENSE")
set(END_MARK "END_LICENSE */")

set(replaced 0)
set(inserted 0)

foreach (relpath IN LISTS FILES)
    set(path "${ROOT_DIR}/${relpath}")
    file(READ "${path}" CONTENT_RAW)

    # Normalize file line endings for processing
    string(REPLACE "\r\n" "\n" CONTENT "${CONTENT_RAW}")
    string(REPLACE "\r" "\n" CONTENT "${CONTENT}")

    # Find begin marker
    string(FIND "${CONTENT}" "${BEGIN_MARK}" BEGIN_POS)

    if (BEGIN_POS GREATER -1)
        # Find end marker, but search starting at BEGIN_POS
        string(SUBSTRING "${CONTENT}" ${BEGIN_POS} -1 TAIL)
        string(FIND "${TAIL}" "${END_MARK}" END_REL_POS)

        if (END_REL_POS EQUAL -1)
            message(WARNING "Found BEGIN_LICENSE but no END_LICENSE in: ${relpath} (skipping)")
            continue()
        endif ()

        # Compute absolute end position
        math(EXPR END_POS "${BEGIN_POS} + ${END_REL_POS}")

        # END_MARK length
        string(LENGTH "${END_MARK}" END_LEN)
        math(EXPR AFTER_END "${END_POS} + ${END_LEN}")

        # Also remove trailing newlines immediately after the end marker (optional cleanup)
        # We'll consume up to two newlines if present.
        string(LENGTH "${CONTENT}" CONTENT_LEN)
        set(AFTER_END2 "${AFTER_END}")
        if (AFTER_END2 LESS CONTENT_LEN)
            string(SUBSTRING "${CONTENT}" ${AFTER_END2} 1 CH1)
            if (CH1 STREQUAL "\n")
                math(EXPR AFTER_END2 "${AFTER_END2} + 1")
                if (AFTER_END2 LESS CONTENT_LEN)
                    string(SUBSTRING "${CONTENT}" ${AFTER_END2} 1 CH2)
                    if (CH2 STREQUAL "\n")
                        math(EXPR AFTER_END2 "${AFTER_END2} + 1")
                    endif ()
                endif ()
            endif ()
        endif ()

        # Build new content = prefix + LICENSE_BLOCK + suffix
        if (BEGIN_POS GREATER 0)
            string(SUBSTRING "${CONTENT}" 0 ${BEGIN_POS} PREFIX)
        else ()
            set(PREFIX "")
        endif ()

        if (AFTER_END2 LESS CONTENT_LEN)
            math(EXPR SUFFIX_LEN "${CONTENT_LEN} - ${AFTER_END2}")
            string(SUBSTRING "${CONTENT}" ${AFTER_END2} ${SUFFIX_LEN} SUFFIX)
        else ()
            set(SUFFIX "")
        endif ()

        set(NEW_CONTENT "${PREFIX}${LICENSE_BLOCK}${SUFFIX}")

        if (NOT NEW_CONTENT STREQUAL CONTENT)
            file(WRITE "${path}" "${NEW_CONTENT}")
            math(EXPR replaced "${replaced}+1")
        endif ()

    else ()
        # No existing block -> insert at top
        set(NEW_CONTENT "${LICENSE_BLOCK}${CONTENT}")
        file(WRITE "${path}" "${NEW_CONTENT}")
        math(EXPR inserted "${inserted}+1")
    endif ()
endforeach ()

message(STATUS "License header update done.")
message(STATUS "Replaced existing blocks: ${replaced}")
message(STATUS "Inserted new blocks:      ${inserted}")
