include_directories(${IncludeI})
link_directories(${LnkLibDirs})
add_compile_definitions($<$<CONFIG:RelWithDebInfo>:__DEBUG__>)

add_definitions(/D_CRT_SECURE_NO_WARNINGS /DODBC_STATIC)
add_executable(dbproc main.cpp 
					  ${CMAKE_SOURCE_DIR}/src/Control/Control.cpp)
find_package(ODBC)

target_link_libraries(dbproc ODBC::ODBC 
							 odbccpp_static.lib 
							 Ws2_32.lib)
set_target_properties(dbproc
	PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/controller_core"
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/controller_core"
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/controller_core"
	)