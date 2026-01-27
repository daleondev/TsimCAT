qt_add_executable(appTsimCAT
    main.cpp
)

target_link_libraries(appTsimCAT
    PRIVATE Qt6::Quick
    TsimCAT          # Links the UI module
    TsimCATplugin    # Links the UI module plugin
    TsimCAT_Backend  # Links the Logic module
    TsimCAT_Backendplugin # Links the Logic module plugin
)
