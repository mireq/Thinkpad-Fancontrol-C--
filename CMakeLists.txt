project(TpFancontrol C CXX)
cmake_minimum_required(VERSION 2.6)
cmake_policy(SET CMP0003 NEW)

set(CMAKE_CXX_FLAGS_RELEASE "-std=c++11 -Wall -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG   "-O0 -g -std=c++11 -Wall -Wextra -DDEBUG")
set(CMAKE_CXX_FLAGS "-std=c++11")

include_directories(
	"${PROJECT_BINARY_DIR}"
	"${CMAKE_CURRENT_SOURCE_DIR}"
	"${CMAKE_CURRENT_BINARY_DIR}"
	${LIBSQLITE3_INCLUDE_DIRS}
)

set(fancontrol_HDRS
	FanControl.h
	Logger.h
)

set(fancontrol_SRCS
	fancontrol.cpp
	FanControl.cpp
	Logger.cpp
)

add_executable(ibm-fancontrold
	${fancontrol_HDRS}
	${fancontrol_SRCS}
)

target_link_libraries(
	ibm-fancontrold
)
