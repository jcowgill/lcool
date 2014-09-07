# CMake script to find LLVM using llvm-config

# Find llvm-config
if (DEFINED LLVM_ROOT)
	find_program(LLVM_CONFIG llvm-config "${LLVM_ROOT}/bin")
endif()
find_program(LLVM_CONFIG NAMES llvm-config)

if (LLVM_CONFIG)
	# Get llvm variables
	execute_process(
		COMMAND "${LLVM_CONFIG}" --version
		OUTPUT_VARIABLE LLVM_VERSION
		OUTPUT_STRIP_TRAILING_WHITESPACE)

	execute_process(
		COMMAND "${LLVM_CONFIG}" --includedir
		OUTPUT_VARIABLE LLVM_INCLUDE_DIRS
		OUTPUT_STRIP_TRAILING_WHITESPACE)

	execute_process(
		COMMAND "${LLVM_CONFIG}" --libdir
		OUTPUT_VARIABLE LLVM_LIBRARY_DIRS
		OUTPUT_STRIP_TRAILING_WHITESPACE)

	execute_process(
		COMMAND "${LLVM_CONFIG}" --bindir
		OUTPUT_VARIABLE LLVM_BIN_DIR
		OUTPUT_STRIP_TRAILING_WHITESPACE)

	find_library(
		LLVM_LIBRARIES
		"LLVM-${LLVM_VERSION}"
		PATHS "${LLVM_LIBRARY_DIRS}")
endif()

# Handle standard args
include(FindPackageHandleStandardArgs)
Find_Package_Handle_Standard_Args(LLVM DEFAULT_MSG LLVM_VERSION LLVM_INCLUDE_DIRS LLVM_LIBRARIES LLVM_BIN_DIR)
