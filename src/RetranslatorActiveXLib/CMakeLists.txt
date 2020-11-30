find_package(IDL REQUIRED)

add_idl(RetranslatorIDL Retranslator.idl)

add_library(retranslatorAX SHARED AXEntry.cpp
                          AXEntry.cpp
                          Factory.cpp
                          Registry.cpp
                          RetranslatorAX.cpp
						  RetranslatorUtilsAX.cpp
						  
						  PipeQueueSystem/PipeQueueSystem.cpp
						  
                          Retranslator.rc
                          Exports.def
                          Hook.cpp
                          )
                          
target_link_libraries(retranslatorAX PUBLIC RetranslatorIDL slogger)
target_include_directories(retranslatorAX PRIVATE ${ProjI})
set_target_properties(retranslatorAX
   PROPERTIES
   ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/retranslatorAX"
   LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/retranslatorAX"
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/retranslatorAX"
   )

target_include_directories(retranslatorAX INTERFACE
   $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
)

#message(${BUILD_TYPE})
add_custom_command(TARGET retranslatorAX 
   POST_BUILD
   COMMAND ${CMAKE_COMMAND} -E copy ARGS ${CMAKE_CURRENT_BINARY_DIR}/Generated/Retranslator.tlb ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/retranslatorAX/$<CONFIG>/Retranslator.tlb
   VERBATIM)

# add_executable(retranslatorAXTestCOM test/test.cpp)
# target_link_libraries(retranslatorAXTestCOM PUBLIC RetranslatorIDL)
# set_target_properties(retranslatorAXTestCOM
   # PROPERTIES
   # ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/retranslatorAX"
   # LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/retranslatorAX"
   # RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/retranslatorAX"
   # )