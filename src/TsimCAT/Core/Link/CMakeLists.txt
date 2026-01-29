add_library(core_link)
add_library(core::link ALIAS core_link)

target_sources(
    core_link
    PRIVATE
    Symbolic/AdsLink.cpp

    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_LIST_DIR}/..
    FILES 
        ILink.hpp
        Subscription.hpp
        Symbolic/ISymbolicLink.hpp
        Symbolic/AdsLink.hpp
)

target_link_libraries(
    core_link
    PRIVATE
    ads::ads

    PUBLIC
    core::common
    core::coroutines
)

target_compile_features(
    core_link 
    PUBLIC 
    cxx_std_23
)
