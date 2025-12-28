cmake_minimum_required(VERSION 3.20)
project(tlink LANGUAGES CXX)

add_library(tlink INTERFACE)
add_library(tlink::tlink ALIAS tlink)

target_include_directories(
    tlink
    INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(tlink INTERFACE cxx_std_23)
