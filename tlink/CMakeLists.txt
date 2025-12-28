cmake_minimum_required(VERSION 3.20)
project(tlink LANGUAGES CXX)

# --- Core Interface Library ---
add_library(tlink INTERFACE)
add_library(tlink::tlink ALIAS tlink)

target_include_directories(
    tlink
    INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(tlink INTERFACE cxx_std_23)

# --- Driver Submodules ---
option(TLINK_BUILD_DRIVER_ADS "Build the ADS protocol driver" ON)

if(TLINK_BUILD_DRIVER_ADS)
    add_subdirectory(drivers/ads)
endif()