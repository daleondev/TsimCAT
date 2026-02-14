add_library(core_simulators)
add_library(core::simulators ALIAS core_simulators)

target_sources(
    core_simulators
    PRIVATE
    LaserSimulator.cpp
    RobotSimulator.cpp
    Kinematics.cpp
    ConveyorSimulator.cpp
    CellFlowOrchestrator.cpp
    GantrySimulator.cpp

    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_LIST_DIR}/..
    FILES 
        ISimulator.hpp
        LaserSimulator.hpp
        RobotSimulator.hpp
        Kinematics.hpp
        ConveyorSimulator.hpp
        CellFlowOrchestrator.hpp
        GantrySimulator.hpp
        Part.hpp
        Components/IComponent.hpp
)

target_link_libraries(
    core_simulators
    PUBLIC
    core::common
    core::coroutines
    core::link
    orocos-kdl
    Eigen3::Eigen
    ompl
)

target_compile_features(
    core_simulators 
    PUBLIC 
    cxx_std_23
)
