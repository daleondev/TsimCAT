add_library(core_logger)
add_library(core::logger ALIAS core_logger)

target_sources(
    core_logger
    PRIVATE
    Logger.cpp
    TraceLogger.cpp

    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_LIST_DIR}/..
    FILES 
        Logger.hpp
        TraceLogger.hpp
)

target_link_libraries(
    core_logger
    PUBLIC
    format_utils::format_utils
    core::common
)

target_compile_features(
    core_logger
    PUBLIC
    cxx_std_23
)
