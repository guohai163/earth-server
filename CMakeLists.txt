cmake_minimum_required (VERSION 3.1)

project (EarthS C)

set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -I/usr/local/include" )
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -I/usr/local/include" )

find_library(LIBEVENT2_CORE event_core)


add_executable (earth_server earth.c)

target_link_libraries(earth_server ${LIBEVENT2_CORE})
