cmake_minimum_required(VERSION 3.12)

project(pwe_core CXX)

find_package(libcommon CONFIG REQUIRED)
find_package(codegen CONFIG REQUIRED)
find_package(tinyxml2 CONFIG REQUIRED)
find_package(nod CONFIG REQUIRED)
find_package(logvisor CONFIG REQUIRED)
find_package(GLEW 2.1 REQUIRED)
find_package(lzokay CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(assimp CONFIG REQUIRED)

find_library(ZLIB_LIBRARY z)

include(codegen)


file(GLOB_RECURSE source_files
    "*.cpp"
    "*.h"
    "*.hpp"
)

add_library(pwe_core ${source_files})

target_compile_features(pwe_core PRIVATE cxx_std_17)

target_include_directories(pwe_core
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/..>
)

target_link_directories(
    pwe_core
    PRIVATE
        ${ASSIMP_LIBRARY_DIRS}
)

target_link_libraries(
    pwe_core
    libcommon::libcommon
    nod::nod
    logvisor::logvisor
    lzokay::lzokay
    OpenGL::GL
    GLEW::GLEW
    codegen::codegen
    "${ZLIB_LIBRARY}"
    ${ASSIMP_LIBRARIES}
)

target_compile_definitions(
    pwe_core
    PRIVATE
        USE_LZOKAY
)

get_target_property(pwecore_include_directories pwe_core INCLUDE_DIRECTORIES)

add_codegen_targets(
    "${source_files}"
    codegen_generated_files
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_BINARY_DIR}"
    "${pwecore_include_directories}"
)
add_custom_target(pwe_core_codegen DEPENDS ${codegen_generated_files} SOURCES ${source_files})

# Add the generated sources to the library target
target_sources(pwe_core PRIVATE ${codegen_generated_files})
