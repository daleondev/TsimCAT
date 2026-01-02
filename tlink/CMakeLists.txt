cmake_minimum_required(VERSION 3.20)
project(tlink LANGUAGES CXX)

include(FetchContent)

FetchContent_Declare(
    magic_enum
    GIT_REPOSITORY https://github.com/Neargye/magic_enum.git
    GIT_TAG        v0.9.7
)
FetchContent_MakeAvailable(magic_enum)

FetchContent_Declare(
    reflect
    GIT_REPOSITORY https://github.com/qlibs/reflect.git
    GIT_TAG        v1.2.6
)
FetchContent_MakeAvailable(reflect)
add_library(reflect INTERFACE)
add_library(reflect::reflect ALIAS reflect)
target_include_directories(
    reflect 
    INTERFACE 
    ${reflect_SOURCE_DIR}
)

add_library(tlink INTERFACE)
add_library(tlink::tlink ALIAS tlink)

target_sources(tlink INTERFACE
    include/tlink/tlink.hpp
    include/tlink/Driver.hpp
    include/tlink/Result.hpp
    include/tlink/Subscription.hpp
    include/tlink/coroutine/coroutine.hpp
    include/tlink/coroutine/Context.hpp
    include/tlink/coroutine/Task.hpp
    include/tlink/coroutine/Channel.hpp
    include/tlink/coroutine/utils.hpp
    include/tlink/log/Logger.hpp
    include/tlink/log/format.hpp
)

target_include_directories(
    tlink
    INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(
    tlink
    INTERFACE
    magic_enum::magic_enum
    reflect::reflect
)

target_compile_features(
    tlink 
    INTERFACE 
    cxx_std_23
)

option(TLINK_BUILD_DRIVER_ADS "Build the ADS protocol driver" OFF)
if(TLINK_BUILD_DRIVER_ADS)
    add_subdirectory(drivers/ads)
endif()

option(TLINK_BUILD_DRIVER_OPCUA "Build the OPC UA protocol driver" OFF)
if(TLINK_BUILD_DRIVER_OPCUA)
    add_subdirectory(drivers/opcua)
endif()