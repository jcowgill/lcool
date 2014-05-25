# CMake build script for lcool

cmake_minimum_required(VERSION 2.8)

# Project and version
project(lcool)
set(LCOOL_VERSION_MAJOR 0)
set(LCOOL_VERSION_MINOR 1)

# Add local modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")

# Enable GCC flags
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR CMAKE_COMPILER_IS_GNUCXX)
	list(APPEND CMAKE_CXX_FLAGS "--std=c++0x -Wall -Wextra -pedantic")
endif()

# Find LLVM and Boost
find_package(Boost REQUIRED COMPONENTS)
find_package(LLVM REQUIRED COMPONENTS support)

# Compile compiler using sources
set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")
add_executable(lcoolc
	"${SRC_DIR}/ast.cpp"
	"${SRC_DIR}/ast.hpp"
	"${SRC_DIR}/parser.cpp"
	"${SRC_DIR}/parser.hpp"
)

# Set options
include_directories(${SRC_DIR} ${BOOST_INCLUDE_DIRS} ${LLVM_INCLUDE_DIRS})
target_link_libraries(lcoolc ${BOOST_LIBRARIES} ${LLVM_LIBRARIES})
