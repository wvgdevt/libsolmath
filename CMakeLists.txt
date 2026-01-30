file(GLOB_RECURSE sources "*.h" "*.cpp")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_library(solmath ${sources})
target_include_directories(solmath PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(solmath PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/")

target_include_directories(solmath PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../deps/magic_enum/include")
target_compile_definitions(solmath PUBLIC SOLMATH_PRECISION=float)


add_custom_target(solmath_update_license
        COMMAND ${CMAKE_COMMAND}
        -DROOT_DIR=${CMAKE_CURRENT_SOURCE_DIR}
        -DTEMPLATE=${CMAKE_CURRENT_SOURCE_DIR}/cmake/license_template.txt
        -DOWNER="winsvega"
        -DYEAR=2026
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/update_license.cmake
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
set_target_properties(solmath_update_license PROPERTIES EXCLUDE_FROM_ALL TRUE)