/*
 * Copyright (C) 2016 James Cowgill
 *
 * LCool is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LCool is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LCool.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <functional>
#include <map>
#include <string>

namespace lcool
{
	namespace test
	{
		// Test status codes
		enum class test_status
		{
			pass,
			fail,
			error,
		};

		// The result of a test
		struct test_result
		{
			test_status status;
			std::string err_msg;
		};

		// Information provided to the test
		struct test_info
		{
			// Path to lcoolc executable
			std::string lcoolc_path;
		};

		// Test function type
		typedef std::function<test_result(const test_info&)> test_fptr;

		// Main list of testcases
		extern const std::map<std::string, test_fptr> testcases;

		// Expected output form build only tests
		enum class build_expect
		{
			// Good tests expect no errors or warnings
			good,

			// Warnings tests ecpect the compile to suceed with some warnings
			warnings,

			// Errors tests expect the compile to fail with some errors
			errors,
		};

		// Build only tests
		//  These tests run the compiler but don't execute the resulting
		//  program. file refers to the path to the test program. The suffix
		//  ".cl" is added to get the input file and the suffix ".out" is added
		//  to get the warn/error file.
		//  - parse tests run the lexer/parser only
		//  - compile tests run the full compile process (to llvm)
		test_fptr parse(const char* file, build_expect expected = build_expect::good);
		test_fptr compile(const char* file, build_expect expected = build_expect::good);

		// Semantic tests
		//  These tests compile and run the given program
		//  The compile must succeed without errors (warnings ignored)
		//  The test must output the right information.
		//  - "input" tests get an input file (.in) piped into them
		test_fptr semantic(const char* file);
		test_fptr semantic_input(const char* file);
	}
}
