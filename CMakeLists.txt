# CMake build script for lcool

cmake_minimum_required(VERSION 2.8)

# Project and version
project(lcool)
set(LCOOL_VERSION_MAJOR 0)
set(LCOOL_VERSION_MINOR 1)

# Add local modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")

# Enable GCC flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -pedantic")

# Find LLVM and Boost
find_package(Boost REQUIRED)
find_package(LLVM 3.5 REQUIRED)

# Enable extended definitions for LLVM
add_definitions(-D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS)

# Compile compiler using sources
set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")
set(BIN_DIR "${CMAKE_BINARY_DIR}")
add_executable(lcoolc
	"${SRC_DIR}/ast.cpp"
	"${SRC_DIR}/ast.hpp"
	"${SRC_DIR}/builtins.cpp"
	"${SRC_DIR}/builtins.hpp"
	"${SRC_DIR}/codegen.cpp"
	"${SRC_DIR}/codegen.hpp"
	"${SRC_DIR}/cool_program.cpp"
	"${SRC_DIR}/cool_program.hpp"
	"${BIN_DIR}/lcool_runtime.inc"
	"${SRC_DIR}/layout.cpp"
	"${SRC_DIR}/layout.hpp"
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

# Assemble builtins file
add_custom_command(OUTPUT "${BIN_DIR}/lcool_runtime.bc"
	DEPENDS "${SRC_DIR}/lcool_runtime.ll"
	COMMAND ${LLVM_BIN_DIR}/llvm-as -o "${BIN_DIR}/lcool_runtime.bc" "${SRC_DIR}/lcool_runtime.ll"
	VERBATIM
)

add_custom_command(OUTPUT "${BIN_DIR}/lcool_runtime.inc"
	DEPENDS "${BIN_DIR}/lcool_runtime.bc"
	COMMAND xxd -i - "${BIN_DIR}/lcool_runtime.inc" < "${BIN_DIR}/lcool_runtime.bc"
	VERBATIM
)

# Set options
include_directories(SYSTEM ${BOOST_INCLUDE_DIRS} ${LLVM_INCLUDE_DIRS})
include_directories(${SRC_DIR} ${BIN_DIR})
target_link_libraries(lcoolc ${BOOST_LIBRARIES} ${LLVM_LIBRARIES})
