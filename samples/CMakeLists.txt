cmake_minimum_required(VERSION 3.20)

add_executable(generator_sample)

target_sources(
    generator_sample 
    PRIVATE 
    generator_sample.cpp
)

target_link_libraries(
    generator_sample
    PRIVATE
    stdc++exp
)

target_compile_features(
    generator_sample 
    PRIVATE 
    cxx_std_23
)

add_executable(task_sample)

target_sources(
    task_sample 
    PRIVATE 
    task_sample.cpp
)

target_link_libraries(
    task_sample
    PRIVATE
    stdc++exp
)

target_compile_features(
    task_sample 
    PRIVATE 
    cxx_std_23
)