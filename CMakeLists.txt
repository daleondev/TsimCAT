cmake_minimum_required(VERSION 3.20)
project(
    TsimCAT
    VERSION 0.1.0
    DESCRIPTION "The Simulated Control and Automation Toolkit"
    LANGUAGES CXX
)

find_package(Threads REQUIRED)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
add_subdirectory(external/ADS/AdsLib)
set_target_properties(ads PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/external/ADS")
target_compile_definitions(ads PRIVATE CONFIG_DEFAULT_LOGLEVEL=1)

set(ELEMENTS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(external/elements)

add_subdirectory(external/json)
add_subdirectory(external/magic_enum)
add_subdirectory(tlink)

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

add_executable(TsimCAT)

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
    ads::ads
    elements
    nlohmann_json::nlohmann_json
    magic_enum::magic_enum
    tlink::tlink
    tlink::ads
)

target_compile_features(
    TsimCAT
    PRIVATE
    cxx_std_23
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

add_subdirectory(samples)