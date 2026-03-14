cmake_minimum_required(VERSION 3.16)

project(TsimCAT VERSION 0.1 LANGUAGES CXX)

if(NOT WIN32)
	message(FATAL_ERROR "TsimCAT is configured as Windows 11 only.")
endif()

if(NOT MSVC)
	message(FATAL_ERROR "TsimCAT requires MSVC toolchain.")
endif()

set(CMAKE_DISABLE_FIND_PACKAGE_WrapVulkanHeaders ON)
find_package(Qt6 REQUIRED COMPONENTS Quick Quick3D Quick3DAssetImport QuickControls2)

qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)
set(QT_NO_PRIVATE_MODULE_WARNING ON)

add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0A00)
add_compile_options(/constexpr:steps20000000)

qt_standard_project_setup()

include(extern/extern.cmake)

enable_testing()

add_subdirectory(src)
add_subdirectory(tests)