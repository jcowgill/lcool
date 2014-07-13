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
find_package(Boost REQUIRED)
find_package(LLVM REQUIRED)

# Enable extended definitions for LLVM
add_definitions(-D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS)

# Compile compiler using sources
set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")
set(BIN_DIR "${CMAKE_BINARY_DIR}")
add_executable(lcoolc
	"${SRC_DIR}/ast.cpp"
	"${SRC_DIR}/ast.hpp"
	"${BIN_DIR}/lcool_runtime.inc"
	"${SRC_DIR}/builtins.cpp"
	"${SRC_DIR}/builtins.hpp"
	"${SRC_DIR}/lexer.cpp"
	"${SRC_DIR}/lexer.hpp"
	"${SRC_DIR}/logger.cpp"
	"${SRC_DIR}/logger.hpp"
	"${SRC_DIR}/main.cpp"
	"${SRC_DIR}/parser.cpp"
	"${SRC_DIR}/parser.hpp"
	"${SRC_DIR}/parser_dump.cpp"
	"${SRC_DIR}/program_structure.cpp"
	"${SRC_DIR}/program_structure.hpp"
	"${SRC_DIR}/smart_ptr.hpp"
)

# Assemble builtins file
add_custom_command(OUTPUT "${BIN_DIR}/lcool_runtime.inc"
	COMMAND llvm-as -o - "${SRC_DIR}/lcool_runtime.ll" | xxd -i - "${BIN_DIR}/lcool_runtime.inc"
	VERBATIM
)

# Set options
include_directories(${SRC_DIR} ${BIN_DIR} ${BOOST_INCLUDE_DIRS} ${LLVM_INCLUDE_DIRS})
target_link_libraries(lcoolc ${BOOST_LIBRARIES} ${LLVM_LIBRARIES})
