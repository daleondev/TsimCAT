add_library(coroutine INTERFACE)
add_library(coroutine::coroutine ALIAS coroutine)

target_sources(
    coroutine
    INTERFACE
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_LIST_DIR}
    FILES 
        coroutine.hpp
        Context.hpp
        Task.hpp
        Channel.hpp
        utils.hpp
)

target_compile_features(
    coroutine 
    INTERFACE 
    cxx_std_23
)