# Lcool Testsuite
#  Currently this runs a load of functional tests over the lcoolc binary

# Test functions
function(_build_test TEST_TYPE TEST_NAME)
	add_test(NAME "test_${TEST_NAME}" COMMAND
		"${CMAKE_CURRENT_SOURCE_DIR}/build_test_driver"
		$<TARGET_FILE:lcoolc> "${TEST_TYPE}" "${TEST_NAME}"
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
endfunction()
function(_semantic_test TEST_TYPE TEST_NAME)
	add_test(NAME "test_${TEST_NAME}" COMMAND
		"${CMAKE_CURRENT_SOURCE_DIR}/semantic_test_driver"
		$<TARGET_FILE:lcoolc> "${LLVM_TOOLS_BINARY_DIR}/lli"
		"${TEST_TYPE}" "${TEST_NAME}"
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
endfunction()

function(test_parse_good TEST_NAME)
	_build_test(parse_good "${TEST_NAME}")
endfunction()
function(test_parse_warn TEST_NAME)
	_build_test(parse_warn "${TEST_NAME}")
endfunction()
function(test_parse_error TEST_NAME)
	_build_test(parse_error "${TEST_NAME}")
endfunction()
function(test_compile_good TEST_NAME)
	_build_test(compile_good "${TEST_NAME}")
endfunction()
function(test_compile_warn TEST_NAME)
	_build_test(compile_warn "${TEST_NAME}")
endfunction()
function(test_compile_error TEST_NAME)
	_build_test(compile_error "${TEST_NAME}")
endfunction()
function(test_semantic TEST_NAME)
	_semantic_test(semantic "${TEST_NAME}")
endfunction()
function(test_compile_error TEST_NAME)
	_semantic_test(semanic_input "${TEST_NAME}")
endfunction()

test_semantic(hello)
