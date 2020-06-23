cmake_minimum_required(VERSION 3.9)
add_compile_definitions($<$<CONFIG:RelWithDebInfo>:__DEBUG__>)
include_directories(${IncludeI})
#set(BoostDateTime "E:/Projects/boost_1_73_0/bin.v2/libs/date_time/build/msvc-14.2/release/link-static/threading-multi")
link_directories(${LnkLibDirs})

add_definitions(/D_CRT_SECURE_NO_WARNINGS /DODBC_STATIC)
find_package(ODBC)
add_library(controller SHARED dllmain.cpp
	dll.cpp 
	fixer.cpp 
	Output.cpp 
	State.cpp 
	Databases.cpp 
	Init.cpp 
	Reader.cpp 
	SerialPool.cpp
	Lights.cpp
	
	encoding/Encode.cpp
	encoding/MD5.cpp
	
	${CMAKE_SOURCE_DIR}/src/Control/Control.cpp)
	
target_link_libraries(controller serial.lib 
								 dllInj.lib 
								 ODBC::ODBC 
								 odbccpp_static.lib 
								 Ws2_32.lib							 
								 mmsg)
								 
set_property(TARGET controller PROPERTY
  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:Debug>DLL")
  
target_compile_options(controller PRIVATE "$<$<CONFIG:Release>:/GL>")
target_link_options(controller PRIVATE "$<$<CONFIG:Release>:/LTCG>")