include_directories(${ProjI})

add_executable(msgb WIN32 msgb.cpp)
set_target_properties(msgb
PROPERTIES
ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/controller_core"
LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/controller_core"
RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/controller_core"
)