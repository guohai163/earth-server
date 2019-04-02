cmake_minimum_required (VERSION 3.1)

project (EarthS)

set(CMAKE_CXX_STANDARD 11)


set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -I/usr/local/include" )
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -I/usr/local/include" )

find_library(LIBEVENT2_CORE event_core)
find_library(LIBS2 s2)

aux_source_directory(src DIR_SRCS)
add_executable (earth_server ${DIR_SRCS})

target_link_libraries(earth_server ${LIBEVENT2_CORE} ${LIBS2})
