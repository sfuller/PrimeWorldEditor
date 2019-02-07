cmake_minimum_required(VERSION 3.12)

project(PrimeWorldEditor CXX)

include(./dew.cmake)
integrate_dew()

add_subdirectory(src/Core)
add_subdirectory(src/Editor)


