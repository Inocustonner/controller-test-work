include_directories(${ProjI})

add_library(mmsg STATIC mmsg.cpp)

add_executable(msgb WIN32 msgb.cpp)