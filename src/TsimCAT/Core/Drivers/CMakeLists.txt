add_library(core_drivers INTERFACE)
add_library(core::drivers ALIAS core_drivers)

target_sources(
    core_drivers
    INTERFACE
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_LIST_DIR}/..
    FILES 
        Comm/IDriver.hpp
        Comm/Symbolic/ISymbolicComm.hpp
        Comm/Raw/IRawComm.hpp
)

target_link_libraries(
    core_drivers
    INTERFACE
    core::coroutines
)

target_compile_features(
    core_drivers 
    INTERFACE 
    cxx_std_23
)
