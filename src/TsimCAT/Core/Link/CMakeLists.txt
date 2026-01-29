add_library(core_link)
add_library(core::link ALIAS core_link)

target_sources(
    core_link
    PRIVATE
    Raw/TcpServer.cpp
    Symbolic/AdsClient.cpp
    Symbolic/OpcUaClient.cpp

    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_LIST_DIR}/..
    FILES 
        ILink.hpp
        Subscription.hpp
        Raw/IRawLink.hpp
        Raw/TcpServer.hpp
        Symbolic/ISymbolicLink.hpp
        Symbolic/AdsClient.hpp
        Symbolic/OpcUaClient.hpp
)

target_link_libraries(
    core_link
    PRIVATE
    ads::ads
    open62541::open62541
    asio::asio

    PUBLIC
    core::common
    core::coroutines
)

target_compile_features(
    core_link 
    PUBLIC 
    cxx_std_23
)
