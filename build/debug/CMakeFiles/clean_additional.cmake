# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "src\\app\\CMakeFiles\\appTsimCAT_autogen.dir\\AutogenUsed.txt"
  "src\\app\\CMakeFiles\\appTsimCAT_autogen.dir\\ParseCache.txt"
  "src\\app\\appTsimCAT_autogen"
  "src\\backend\\CMakeFiles\\TsimCAT_Backend_autogen.dir\\AutogenUsed.txt"
  "src\\backend\\CMakeFiles\\TsimCAT_Backend_autogen.dir\\ParseCache.txt"
  "src\\backend\\CMakeFiles\\TsimCAT_Backendplugin_autogen.dir\\AutogenUsed.txt"
  "src\\backend\\CMakeFiles\\TsimCAT_Backendplugin_autogen.dir\\ParseCache.txt"
  "src\\backend\\CMakeFiles\\TsimCAT_Backendplugin_init_autogen.dir\\AutogenUsed.txt"
  "src\\backend\\CMakeFiles\\TsimCAT_Backendplugin_init_autogen.dir\\ParseCache.txt"
  "src\\backend\\TsimCAT_Backend_autogen"
  "src\\backend\\TsimCAT_Backendplugin_autogen"
  "src\\backend\\TsimCAT_Backendplugin_init_autogen"
  "src\\frontend\\CMakeFiles\\TsimCAT_autogen.dir\\AutogenUsed.txt"
  "src\\frontend\\CMakeFiles\\TsimCAT_autogen.dir\\ParseCache.txt"
  "src\\frontend\\CMakeFiles\\TsimCATplugin_autogen.dir\\AutogenUsed.txt"
  "src\\frontend\\CMakeFiles\\TsimCATplugin_autogen.dir\\ParseCache.txt"
  "src\\frontend\\CMakeFiles\\TsimCATplugin_init_autogen.dir\\AutogenUsed.txt"
  "src\\frontend\\CMakeFiles\\TsimCATplugin_init_autogen.dir\\ParseCache.txt"
  "src\\frontend\\TsimCAT_autogen"
  "src\\frontend\\TsimCATplugin_autogen"
  "src\\frontend\\TsimCATplugin_init_autogen"
  )
endif()
