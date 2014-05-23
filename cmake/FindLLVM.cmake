# CMake script to find LLVM using llvm-config

# Find llvm-config
if (DEFINED LLVM_ROOT)
	find_program(LLVM_CONFIG llvm-config "${LLVM_ROOT}/bin")
endif()
find_program(LLVM_CONFIG NAMES llvm-config llvm-config-3.1)

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
		COMMAND "${LLVM_CONFIG}" --libfiles ${LLVM_FIND_COMPONENTS}
		OUTPUT_VARIABLE LLVM_LIBRARIES
		OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()

# Handle standard args
include(FindPackageHandleStandardArgs)
Find_Package_Handle_Standard_Args(LLVM DEFAULT_MSG LLVM_VERSION LLVM_INCLUDE_DIRS LLVM_LIBRARIES)
