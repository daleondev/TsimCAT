qt_add_library(TsimCAT STATIC)

target_link_libraries(TsimCAT PRIVATE Qt6::Quick)

qt_add_qml_module(TsimCAT
    URI "TsimCAT"
    VERSION 1.0
    QML_FILES
        Main.qml
)
