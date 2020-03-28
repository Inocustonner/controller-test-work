cmake_minimum_required(VERSION 3.9)
include_directories("${CMAKE_SOURCE_DIR}/src")
include_directories("${CMAKE_SOURCE_DIR}/include")
link_directories("${CMAKE_SOURCE_DIR}/lib/${CMAKE_BUILD_TYPE}")


add_definitions(/D_CRT_SECURE_NO_WARNINGS)
find_package(ODBC)
add_library(controller SHARED dllmain.cpp dll.cpp fixer.cpp)
target_link_libraries(controller dllInj.lib ODBC::ODBC odbc_lib p7.lib Ws2_32.lib)
set_property(TARGET controller PROPERTY
  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:Debug>DLL")

#force c++latest on msvc  
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++latest")