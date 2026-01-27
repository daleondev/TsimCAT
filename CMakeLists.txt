cmake_minimum_required(VERSION 3.16)

project(TsimCAT VERSION 0.1 LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Quick)

qt_policy(SET QTP0001 NEW)

qt_standard_project_setup()

add_subdirectory(src)