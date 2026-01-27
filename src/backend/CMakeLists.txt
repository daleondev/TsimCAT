qt_add_library(TsimCAT_Backend STATIC
    Backend.h
    Backend.cpp
)

target_link_libraries(TsimCAT_Backend PUBLIC Qt6::Quick)

# Define the module URI for QML
qt_add_qml_module(TsimCAT_Backend
    URI "TsimCAT.Backend"
    VERSION 1.0
    SOURCES
        Backend.h
        Backend.cpp
)
