file(GLOB_RECURSE sources "*.h" "*.cpp")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_library(solmath ${sources})
target_include_directories(solmath PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(solmath PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/")

target_include_directories(solmath PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../deps/magic_enum/include")
target_compile_definitions(solmath PUBLIC SOLMATH_PRECISION=float)
