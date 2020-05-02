cmake_minimum_required(VERSION 3.9)
include_directories("${CMAKE_SOURCE_DIR}/src")
include_directories("${CMAKE_SOURCE_DIR}/include")
link_directories("${CMAKE_SOURCE_DIR}/lib/${CMAKE_BUILD_TYPE}")

add_subdirectory(mmsg)

add_definitions(/D_CRT_SECURE_NO_WARNINGS /DODBC_STATIC)
find_package(ODBC)
add_library(controller SHARED dllmain.cpp dll.cpp fixer.cpp Output.cpp State.cpp Databases.cpp Init.cpp)
target_link_libraries(controller serial.lib dllInj.lib ODBC::ODBC odbccpp_static.lib Ws2_32.lib mmsg)
set_property(TARGET controller PROPERTY
  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:Debug>DLL")