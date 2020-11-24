add_library(slogger STATIC SLogger.cpp)
target_include_directories(slogger INTERFACE
   $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)