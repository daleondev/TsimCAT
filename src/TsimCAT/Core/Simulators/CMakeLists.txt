add_library(core_simulators INTERFACE)
add_library(core::simulators ALIAS core_simulators)

target_sources(
    core_simulators
    INTERFACE
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_LIST_DIR}/..
    FILES 
        Simulators/ISimulator.hpp
        Simulators/Components/IComponent.hpp
)

target_link_libraries(
    core_simulators
    INTERFACE
    core::coroutines
    core::drivers
)

target_compile_features(
    core_simulators 
    INTERFACE 
    cxx_std_23
)
