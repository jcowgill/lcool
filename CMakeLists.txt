# CMake build script for lcool

cmake_minimum_required(VERSION 2.8)

# Project and version
project(lcool)
set(LCOOL_VERSION_MAJOR 0)
set(LCOOL_VERSION_MINOR 1)

# Add local modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")

# Enable GCC flags
list(APPEND CMAKE_CXX_FLAGS "-std=c++11 -Wall -Wextra -pedantic")

# Find LLVM and Boost
find_package(Boost REQUIRED COMPONENTS)
find_package(LLVM REQUIRED COMPONENTS core)

# Enable extended definitions for LLVM
add_definitions(-D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS)

# Compile compiler using sources
set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")
add_executable(lcoolc
	"${SRC_DIR}/ast.cpp"
	"${SRC_DIR}/ast.hpp"
	"${SRC_DIR}/lexer.cpp"
	"${SRC_DIR}/lexer.hpp"
	"${SRC_DIR}/logger.cpp"
	"${SRC_DIR}/logger.hpp"
	"${SRC_DIR}/main.cpp"
	"${SRC_DIR}/parser.cpp"
	"${SRC_DIR}/parser.hpp"
	"${SRC_DIR}/parser_dump.cpp"
	"${SRC_DIR}/smart_ptr.hpp"
)

# Set options
include_directories(${SRC_DIR} ${BOOST_INCLUDE_DIRS} ${LLVM_INCLUDE_DIRS})
target_link_libraries(lcoolc ${BOOST_LIBRARIES} ${LLVM_LIBRARIES})
