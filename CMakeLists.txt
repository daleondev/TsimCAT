cmake_minimum_required(VERSION 3.20)
project(
    TsimCAT
    VERSION 0.1.0
    DESCRIPTION "The Simulated Control and Automation Toolkit"
    LANGUAGES CXX
)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
add_subdirectory(external/ADS)
add_subdirectory(external/elements)
add_subdirectory(external/json)
add_subdirectory(external/magic_enum)

add_executable(
    TsimCAT
    main.cpp
)

target_include_directories(
    TsimCAT
    PRIVATE
)

target_link_libraries(
    TsimCAT
    PRIVATE
    stdc++exp
    ads::ads
    elements
    nlohmann_json::nlohmann_json
    magic_enum::magic_enum
)

target_compile_features(
    TsimCAT
    PRIVATE
    cxx_std_23
)