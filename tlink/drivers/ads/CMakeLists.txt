cmake_minimum_required(VERSION 3.20)

add_library(tlink_ads STATIC)
add_library(tlink::ads ALIAS tlink_ads)

# Source files
target_sources(tlink_ads PRIVATE 
    src/ads.cpp
    include/tlink/drivers/ads.hpp
)

# Public headers
target_include_directories(
    tlink_ads
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Dependencies
target_link_libraries(
    tlink_ads
    PUBLIC
    tlink::tlink
    ads::ads
)

target_compile_features(tlink_ads PUBLIC cxx_std_23)