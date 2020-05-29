set(BoostI "E:/Projects/boost_1_73_0")
set(ProjI "${CMAKE_SOURCE_DIR}/include;${CMAKE_SOURCE_DIR}/src/Control")
set(IncludeI "${BoostI};${ProjI}")

set(BoostDateTime "E:/Projects/boost_1_73_0/bin.v2/libs/date_time/build/msvc-14.2/release/link-static/threading-multi")
set(ProjLibDir "${CMAKE_SOURCE_DIR}/lib/${CMAKE_BUILD_TYPE}")
set(LnkLibDirs "${BoostDateTime};${ProjLibDir}")

add_subdirectory(Control)
add_subdirectory(ControllerDll)
add_subdirectory(mmsg)
add_subdirectory(dbproc)
add_subdirectory(starter)