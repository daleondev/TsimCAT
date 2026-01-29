qt_add_executable(appTsimCAT
    main.cpp
)

target_compile_features(appTsimCAT PRIVATE cxx_std_23)

target_link_libraries(appTsimCAT
    PRIVATE Qt6::Quick
    Qt6::QuickControls2
    TsimCAT_UI
    TsimCAT_UIplugin
    TsimCAT_Backend
    TsimCAT_Backendplugin
)