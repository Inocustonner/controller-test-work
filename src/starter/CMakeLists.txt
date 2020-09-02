add_compile_definitions($<$<CONFIG:RelWithDebInfo>:__DEBUG__>)
add_executable(starter main.cpp 
					   ${CMAKE_SOURCE_DIR}/src/Control/Control.cpp)
target_include_directories(starter PRIVATE ${IncludeI})
target_link_directories(starter PRIVATE ${LnkLibDirs})
set_target_properties(starter
	PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/controller_core"
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/controller_core"
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/controller_core"
	)