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
if(LLVM_VERSION VERSION_LESS 3.8)
	message(FATAL_ERROR "You need LLVM >= 3.5 to build ${CMAKE_PROJECT_NAME}")
elseif(NOT LLVM_VERSION VERSION_LESS 3.9)
	message(WARNING "LLVM ${LLVM_VERSION} has not been tested")
endif()

# Add subdirectories
enable_testing()
add_subdirectory(src)
add_subdirectory(test)
