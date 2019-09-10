cmake_minimum_required(VERSION 3.12)
project(MoonFlower)

find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

find_library(REFLEX_LIB reflex)

add_library(interp
    src/interp.cpp)
set_target_properties(interp PROPERTIES CXX_STANDARD 17)

add_executable(moonflower
    src/main.cpp)
set_target_properties(moonflower PROPERTIES CXX_STANDARD 17)
target_link_libraries(moonflower interp)

flex_target(asmlexer src/asmlexer.l ${CMAKE_CURRENT_BINARY_DIR}/asmlexer.cpp
    DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/asmlexer.hpp)
bison_target(asmparser src/asmparser.y ${CMAKE_CURRENT_BINARY_DIR}/asmparser.cpp
    DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/asmparser.hpp)
add_flex_bison_dependency(asmlexer asmparser)

add_executable(mfasm
    ${BISON_asmparser_OUTPUTS}
    ${FLEX_asmlexer_OUTPUTS}
    src/mfasm.cpp
    src/assembler.cpp)
set_target_properties(mfasm PROPERTIES CXX_STANDARD 17)
target_include_directories(mfasm PUBLIC src ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(mfasm ${REFLEX_LIB})
