cmake_minimum_required(VERSION 3.20)

add_library(tlink_opcua STATIC)
add_library(tlink::opcua ALIAS tlink_opcua)

target_sources(
    tlink_opcua 
    PRIVATE 
    src/OpcUa.cpp
    include/tlink/drivers/OpcUa.hpp
)

target_include_directories(
    tlink_opcua
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(
    tlink_opcua
    PUBLIC
    tlink::tlink
    open62541::open62541
    magic_enum::magic_enum
)

target_compile_features(
    tlink_opcua 
    PUBLIC 
    cxx_std_23
)