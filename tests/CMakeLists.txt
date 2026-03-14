include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.17.0
    GIT_SHALLOW    TRUE
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
include(GoogleTest)

add_executable(simulator_tests
    SimulatorTests.cpp
)

target_link_libraries(simulator_tests
    PRIVATE
    GTest::gtest_main
    core::simulators
)

target_compile_features(simulator_tests PRIVATE cxx_std_23)

gtest_discover_tests(simulator_tests)
