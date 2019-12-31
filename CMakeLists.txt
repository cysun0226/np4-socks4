cmake_minimum_required(VERSION 2.6)
project(sockServer)

FIND_PACKAGE( Boost 1.70.0 COMPONENTS program_options REQUIRED )
INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}-pthread")

add_executable(sockServer socks_server.cpp socks4.cpp parse.h parse.cpp)
TARGET_LINK_LIBRARIES(sockServer LINK_PUBLIC ${Boost_LIBRARIES})
