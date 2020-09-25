add_compile_definitions($<$<CONFIG:RelWithDebInfo>:__DEBUG__>)
include_directories(${IncludeI})
link_directories(${LnkLibDirs})

add_library(ControlLib STATIC Control.cpp)
target_include_directories(ControlLib INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>)