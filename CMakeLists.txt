# CMake build script for lcool

cmake_minimum_required(VERSION 2.8)

# Project and version
project(lcool)
set(LCOOL_VERSION_MAJOR 0)
set(LCOOL_VERSION_MINOR 1)

# Add local modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")

# Enable C++11 and warnings
if(UNIX)
	list(APPEND CMAKE_CXX_FLAGS "-std=c++11 -Wall -Wextra -pedantic")
endif()

# Find LLVM and Boost
find_package(Boost REQUIRED COMPONENTS)
find_package(LLVM REQUIRED COMPONENTS support)

# Compile compiler using sources
set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")
add_executable(lcoolc
	"${SRC_DIR}/blob.cpp"
)

# Set options
include_directories(${BOOST_INCLUDE_DIRS} ${LLVM_INCLUDE_DIRS})
target_link_libraries(lcoolc ${BOOST_LIBRARIES} ${LLVM_LIBRARIES})
