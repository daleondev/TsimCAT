add_library(core_common INTERFACE)
add_library(core::common ALIAS core_common)

target_sources(
    core_common
    INTERFACE
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_LIST_DIR}/..
    FILES 
        Result.hpp
)

target_link_libraries(
    core_common
    INTERFACE
    format_utils::format_utils
)

target_compile_features(
    core_common 
    INTERFACE 
    cxx_std_23
)
