# Replacement for LLVMConfig.cmake on Ubuntu where it is completely broken
set(LLVM_ROOT "" CACHE PATH "Path to LLVM root (eg /usr/lib/llvm-3.8)")
if (NOT LLVM_ROOT)
	message(FATAL_ERROR "If BROKEN_LLVM_UBUNTU is set, LLVM_ROOT must also be set")
endif()

message(STATUS "BrokenLLVM: enabled")

set(LLVM_TOOLS_BINARY_DIR "${LLVM_ROOT}/bin")
set(LLVM_INCLUDE_DIRS "${LLVM_ROOT}/include")
set(LLVM_DEFINITIONS "-D__STDC_LIMIT_MACROS" "-D__STDC_CONSTANT_MACROS")

execute_process(
	COMMAND "${LLVM_TOOLS_BINARY_DIR}/llvm-config" --version
	OUTPUT_VARIABLE LLVM_VERSION
	OUTPUT_STRIP_TRAILING_WHITESPACE)

message(STATUS "BrokenLLVM: found version ${LLVM_VERSION}")

function(llvm_map_components_to_libnames out_libs)
	execute_process(
		COMMAND "${LLVM_TOOLS_BINARY_DIR}/llvm-config" --libfiles ${ARGN}
		OUTPUT_VARIABLE out_libs_loc
		OUTPUT_STRIP_TRAILING_WHITESPACE)
	string(REPLACE " " ";" out_libs_loc "${out_libs_loc}")
	list(APPEND out_libs_loc z pthread ffi tinfo dl m)
	set(${out_libs} ${out_libs_loc} PARENT_SCOPE)
endfunction()
