cmake_minimum_required(VERSION 3.12)
project(MoonFlower)

find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

find_library(REFLEX_LIB reflex)

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

flex_target(scriptlexer src/scriptlexer.l ${CMAKE_CURRENT_BINARY_DIR}/scriptlexer.cpp
    DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/scriptlexer.hpp)
bison_target(scriptparser src/scriptparser.y ${CMAKE_CURRENT_BINARY_DIR}/scriptparser.cpp
    DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/scriptparser.hpp)
add_flex_bison_dependency(scriptlexer scriptparser)

add_library(scriptcompiler
    ${BISON_scriptparser_OUTPUTS}
    ${FLEX_scriptlexer_OUTPUTS})
set_target_properties(scriptcompiler PROPERTIES CXX_STANDARD 17)
target_include_directories(scriptcompiler PUBLIC src ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(scriptcompiler ${REFLEX_LIB})

add_executable(moonflower
    src/main.cpp
    src/interp.cpp
    src/state.cpp
    src/compile.cpp)
set_target_properties(moonflower PROPERTIES CXX_STANDARD 17)
target_link_libraries(moonflower scriptcompiler)

add_executable(mfsc
    src/mfsc.cpp)
set_target_properties(mfsc PROPERTIES CXX_STANDARD 17)
target_link_libraries(mfsc scriptcompiler)

add_executable(mfdisass src/mfdisass.cpp)
set_target_properties(mfdisass PROPERTIES CXX_STANDARD 17)
