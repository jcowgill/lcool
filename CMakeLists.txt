# CMake build script for lcool

cmake_minimum_required(VERSION 2.8)

# Project and version
project(lcool)
set(LCOOL_VERSION_MAJOR 0)
set(LCOOL_VERSION_MINOR 1)

# Enable GCC flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -pedantic")

# Find Boost
find_package(Boost REQUIRED COMPONENTS program_options)

# Find LLVM
option(BROKEN_UBUNTU_LLVM "Set if you have broken Ubuntu LLVM cmake scripts (pre-xenial)")
if (BROKEN_UBUNTU_LLVM)
	include(cmake/BrokenLLVM.cmake)
else()
	find_package(LLVM REQUIRED CONFIG)
endif()

# Validate LLVM version
if(LLVM_VERSION VERSION_LESS 3.5)
	message(FATAL_ERROR "You need LLVM >= 3.5 to build ${CMAKE_PROJECT_NAME}")
elseif(NOT LLVM_VERSION VERSION_LESS 3.6)
	message(WARNING "LLVM ${LLVM_VERSION} has not been tested")
endif()

# Add library include directories and definitions
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# Compile compiler using sources
set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")
set(BIN_DIR "${CMAKE_BINARY_DIR}")

include_directories(${SRC_DIR} ${BIN_DIR})
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
	COMMAND ${LLVM_TOOLS_BINARY_DIR}/llvm-as -o "${BIN_DIR}/lcool_runtime.bc" "${SRC_DIR}/lcool_runtime.ll"
	VERBATIM
)

add_custom_command(OUTPUT "${BIN_DIR}/lcool_runtime.inc"
	DEPENDS "${BIN_DIR}/lcool_runtime.bc"
	COMMAND xxd -i - "${BIN_DIR}/lcool_runtime.inc" < "${BIN_DIR}/lcool_runtime.bc"
	VERBATIM
)

# Link libraries
llvm_map_components_to_libnames(llvm_libs bitreader bitwriter core linker support)
target_link_libraries(lcoolc ${Boost_LIBRARIES} ${llvm_libs})
