add_library(network INTERFACE)
add_library(network::network ALIAS network)

target_sources(
    network
    INTERFACE
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_LIST_DIR}
    FILES 
        IDriver.hpp
)

target_link_libraries(
    network
    INTERFACE
    coroutine::coroutine
)

target_compile_features(
    network 
    INTERFACE 
    cxx_std_23
)