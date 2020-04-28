include_directories("${CMAKE_SOURCE_DIR}/include")

add_library(mmsg STATIC mmsg.cpp)

add_executable(msgb WIN32 msgb.cpp)