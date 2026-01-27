qt_add_library(TsimCAT_UI STATIC)

target_compile_features(TsimCAT_UI PUBLIC cxx_std_23)
target_link_libraries(TsimCAT_UI PRIVATE Qt6::Quick)

qt_add_qml_module(TsimCAT_UI
    URI "TsimCAT.UI"
    VERSION 1.0
    RESOURCE_PREFIX "/qt/qml"
    QML_FILES
        Main.qml
)