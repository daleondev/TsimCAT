set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# QCoro
set(QCORO_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(QCORO_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(QCORO_WITH_QTWEBSOCKETS OFF CACHE BOOL "" FORCE)
set(QCORO_WITH_QTNETWORK OFF CACHE BOOL "" FORCE)
set(QCORO_WITH_QTDBUS OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/qcoro)

# format_utils
set(BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/format_utils)

# ADS
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/ADS/AdsLib)
set_target_properties(ads PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/extern/ADS")
target_compile_definitions(ads PRIVATE CONFIG_DEFAULT_LOGLEVEL=1)

# Open62541
set(UA_ENABLE_AMALGAMATION ON CACHE BOOL "" FORCE)
set(UA_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(UA_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(UA_BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/open62541)

# asio
add_library(asio INTERFACE)
add_library(asio::asio ALIAS asio)
target_include_directories(
    asio
    INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/asio/asio/include
)

# Eigen
add_library(eigen INTERFACE)
add_library(Eigen3::Eigen ALIAS eigen)
target_include_directories(eigen INTERFACE ${CMAKE_CURRENT_LIST_DIR}/eigen)

# KDL
set(EIGEN3_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/eigen CACHE PATH "" FORCE)
set(ENABLE_TESTS OFF CACHE BOOL "" FORCE)
set(ENABLE_EXAMPLES OFF CACHE BOOL "" FORCE)
set(KDL_USE_NEW_TREE_INTERFACE OFF CACHE BOOL "" FORCE) # Avoid Boost dependency
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/kdl/orocos_kdl)
# Force KDL to be static and use our Eigen
set_target_properties(orocos-kdl PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(orocos-kdl PRIVATE eigen)
target_include_directories(orocos-kdl PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/kdl/orocos_kdl/src>")