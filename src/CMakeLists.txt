set(BoostI $ENV{BoostRootDir})
set(ProjI "${CMAKE_SOURCE_DIR}/include")
set(IncludeI "${BoostI};${ProjI}")
#architecture=x86 release runtime-debugging=on variant=debug
#
set(BoostDateTime "$ENV{BoostBinDir}/date_time/build/msvc-14.2/release/link-static/threading-multi")
set(ProjLibDir "${CMAKE_SOURCE_DIR}/lib/${CMAKE_BUILD_TYPE}")
set(LnkLibDirs "${BoostDateTime};${ProjLibDir}")

# add_subdirectory(Control)
add_subdirectory(RetranslatorActiveXLib)
add_subdirectory(ControllerDll)
add_subdirectory(mmsg)
# add_subdirectory(dbproc)
# add_subdirectory(starter)