qt_add_library(TsimCAT_Backend STATIC)

target_link_libraries(TsimCAT_Backend 
    PUBLIC 
    Qt6::Quick
    QCoro6::Core
    QCoro6::Quick
    core::coroutines
    core::link
    core::simulators
)

target_compile_features(
    TsimCAT_Backend 
    PUBLIC 
    cxx_std_23
)

qt_add_qml_module(TsimCAT_Backend
    URI "TsimCAT.Backend"
    VERSION 1.0
    RESOURCE_PREFIX "/qt/qml"
    SOURCES
        Backend.h
        Backend.cpp
)