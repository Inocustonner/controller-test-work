include_directories(${IncludeI})
link_directories(${LnkLibDirs})

add_definitions(/D_CRT_SECURE_NO_WARNINGS /DODBC_STATIC)
add_executable(dbproc main.cpp 
					  ${CMAKE_SOURCE_DIR}/src/Control/Control.cpp)
find_package(ODBC)

target_link_libraries(dbproc ODBC::ODBC 
							 odbccpp_static.lib 
							 Ws2_32.lib)