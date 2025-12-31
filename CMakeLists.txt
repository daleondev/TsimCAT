cmake_minimum_required(VERSION 3.20)
project(
    TsimCAT
    VERSION 0.1.0
    DESCRIPTION "The Simulated Control and Automation Toolkit"
    LANGUAGES CXX
)

# --- Libraries ---

find_package(Threads REQUIRED)

option(TLINK_ADS_SHARED "Build ADS driver as shared library" OFF)
set(BUILD_SHARED_LIBS ${TLINK_ADS_SHARED} CACHE BOOL "" FORCE)
add_subdirectory(external/ADS/AdsLib)
set_target_properties(ads PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/external/ADS")
target_compile_definitions(ads PRIVATE CONFIG_DEFAULT_LOGLEVEL=1)

option(TLINK_OPCUA_SHARED "Build OPC UA driver as shared library" OFF)
set(BUILD_SHARED_LIBS ${TLINK_OPCUA_SHARED} CACHE BOOL "" FORCE)
set(UA_ENABLE_AMALGAMATION ON CACHE BOOL "" FORCE)
set(UA_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(UA_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(UA_BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(external/open62541)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

set(TLINK_BUILD_DRIVER_ADS ON CACHE BOOL "" FORCE)
set(TLINK_BUILD_DRIVER_OPCUA ON CACHE BOOL "" FORCE)
add_subdirectory(tlink)

set(ELEMENTS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(external/elements)

add_subdirectory(external/json)
add_subdirectory(external/magic_enum)

add_library(asio INTERFACE)
add_library(asio::asio ALIAS asio)
target_include_directories(
    asio
    INTERFACE
    external/asio/include
)

# --- Application ---

add_executable(TsimCAT)

file(
    GLOB_RECURSE SOURCES
    CONFIGURE_DEPENDS
    src/*.cpp
)

file(
    GLOB_RECURSE HEADERS
    CONFIGURE_DEPENDS
    src/*.h
    src/*.hpp
)

target_sources(
    TsimCAT
    PRIVATE
    ${SOURCES}
    ${HEADERS}
)

target_include_directories(
    TsimCAT
    PRIVATE
    src
)

target_link_libraries(
    TsimCAT
    PRIVATE
    stdc++exp
    tlink::ads
    tlink::opcua
    elements
    nlohmann_json::nlohmann_json
    asio::asio
)

target_compile_features(
    TsimCAT
    PRIVATE
    cxx_std_23
)

add_custom_command(TARGET TsimCAT POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:TsimCAT> $<TARGET_RUNTIME_DLLS:TsimCAT>
    COMMAND_EXPAND_LISTS
)

foreach(h IN LISTS HEADERS)
    set_source_files_properties(${h} PROPERTIES HEADER_FILE_ONLY TRUE)
endforeach()

source_group(
    TREE ${CMAKE_CURRENT_SOURCE_DIR}/src
    PREFIX "Source Files"
    FILES ${SOURCES}
)

source_group(
    TREE ${CMAKE_CURRENT_SOURCE_DIR}/src
    PREFIX "Header Files"
    FILES ${HEADERS}
)

# --- Samples ---

add_subdirectory(samples)