cmake_minimum_required(VERSION 3.12)

project(pwe_editor LANGUAGES CXX VERSION 1.2.1)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
find_package(Qt5 COMPONENTS Core Widgets REQUIRED)
find_package(assimp CONFIG REQUIRED)

# AssImp's cmake config is pretty awful. It doesn't include necesary libraries. Hopefully this can be fixed later.
find_library(IIRXML_LIBRARY IrrXMLd IrrXML)

file(GLOB_RECURSE source_files
    "*.cpp"
    "*.h"
    "*.hpp"
)

add_executable(pwe_editor ${source_files})

target_compile_features(pwe_editor PRIVATE cxx_std_17)

target_link_directories(
    pwe_editor
    PRIVATE
    ${ASSIMP_LIBRARY_DIRS}
)

target_link_libraries(
    pwe_editor
    pwe_core
    Qt5::Widgets
    ${ASSIMP_LIBRARIES}
    "${IIRXML_LIBRARY}"
)

# TODO: These should be drive by CI if CI ever becomes a thing for PWE.
set(PWE_APP_NAME "Prime World Editor")

target_compile_definitions(
    pwe_editor
    PRIVATE
        "APP_NAME=${PWE_APP_NAME}"
        "APP_VERSION=${PROJECT_VERSION}"
        "PUBLIC_RELEASE=1"
        "APP_FULL_NAME=${PWE_APP_NAME} v${PROJECT_VERSION}"
)
