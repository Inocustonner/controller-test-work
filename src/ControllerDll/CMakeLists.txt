cmake_minimum_required(VERSION 3.9)
add_compile_definitions($<$<CONFIG:RelWithDebInfo>:__DEBUG__>)
include_directories(${IncludeI})
#set(BoostDateTime "E:/Projects/boost_1_73_0/bin.v2/libs/date_time/build/msvc-14.2/release/link-static/threading-multi")
link_directories(${LnkLibDirs})

add_definitions(/D_CRT_SECURE_NO_WARNINGS /DODBC_STATIC)
find_package(ODBC)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

add_library(controller_core STATIC 
	core/encoding/Encode.cpp
	core/encoding/MD5.cpp
	
	core/fixer.cpp 
	core/Output.cpp 
	core/State.cpp 
	core/Init.cpp 
	core/mmsg.cpp)
	
target_link_libraries(controller_core PUBLIC retranslatorAX)
									# Control
									# serial.lib 
								  # dllInj.lib 
								  # ODBC::ODBC 
								  # odbccpp_static.lib
								  # Ws2_32.lib)
target_include_directories(controller_core INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/core>
)
set_target_properties(controller_core
PROPERTIES
ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/controller_core"
LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/controller_core"
RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/controller_core"
)

#controller_standalone
add_executable(controller_standalone standalone/main.cpp
									 standalone/Retranslator.cpp
									 standalone/Commands.cpp)

target_link_libraries(controller_standalone PRIVATE controller_core serial.lib)

set_target_properties(controller_standalone
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/controller_standalone"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/controller_standalone"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/controller_standalone"
	)
#target_compile_options(controller_standalone PRIVATE "$<$<CONFIG:Release>:/GL>")
#target_link_options(controller_standalone PRIVATE "$<$<CONFIG:Release>:/LTCG>")