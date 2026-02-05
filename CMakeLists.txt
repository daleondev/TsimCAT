cmake_minimum_required(VERSION 3.16)

project(TsimCAT VERSION 0.1 LANGUAGES CXX)

set(CMAKE_DISABLE_FIND_PACKAGE_WrapVulkanHeaders ON)
find_package(Qt6 REQUIRED COMPONENTS Quick Quick3D Quick3DAssetImport QuickControls2)

qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)
set(QT_NO_PRIVATE_MODULE_WARNING ON)

qt_standard_project_setup()

include(extern/extern.cmake)

add_subdirectory(src)