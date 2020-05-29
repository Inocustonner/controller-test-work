add_executable(starter main.cpp 
					   ${CMAKE_SOURCE_DIR}/src/Control/Control.cpp)
target_include_directories(starter PRIVATE ${IncludeI})
target_link_directories(starter PRIVATE ${LnkLibDirs})
