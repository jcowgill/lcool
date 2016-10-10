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
#include <iostream>
#include <map>
#include <string>

#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "test.hpp"

using lcool::test::build_expect;
using lcool::test::test_fptr;
using lcool::test::test_info;
using lcool::test::test_result;
using lcool::test::test_status;

// If the child reports this exit code, there was an error during exec
#define MAGIC_ERROR_STATUS 125

// Returns an error test status with errno appended to it
static test_result test_error_with_errno(std::string prefix)
{
	return { test_status::error, prefix + " (" + strerror(errno) + ")" };
}

// Reads contents of file desciptor into a string
//  Returns a pass status with the string, or an error status with the error
//  message
static test_result read_fd(int fd)
{
	test_result result = { test_status::pass, "" };
	char buffer[1024];
	ssize_t bytes_read;

	while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0)
		result.err_msg.append(buffer, bytes_read);

	if (bytes_read < 0)
		return test_error_with_errno("read: ");

	return result;
}

// Reads an entire file into a string
static test_result read_file(const std::string& filename)
{
	int fd = open(filename.c_str(), O_RDONLY);
	if (fd < 0)
		return test_error_with_errno(std::string("open ") + filename + ": ");

	test_result result = read_fd(fd);
	close(fd);
	return result;
}

// Build only test implementation
static test_fptr build_only_test(const char* name, build_expect expected, const char* lcoolc_option)
{
	return [=](const test_info& info) -> test_result {
		// Read expected output file
		std::string expected_output;
		if (expected != build_expect::good)
		{
			test_result out_file = read_file(std::string(name) + ".out");
			if (out_file.status == test_status::error)
				return out_file;

			expected_output = std::move(out_file.err_msg);
		}

		// Fork lcoolc process
		int stderr_pipe[2];
		if (pipe(stderr_pipe) != 0)
			return test_error_with_errno("pipe: ");

		pid_t child = fork();
		if (child == -1)
		{
			close(stderr_pipe[0]);
			close(stderr_pipe[1]);
			return test_error_with_errno("fork: ");
		}

		if (child == 0)
		{
			// Close old file descriptors
			close(STDIN_FILENO);
			close(STDOUT_FILENO);
			close(STDERR_FILENO);
			close(stderr_pipe[0]);

			// Replace stdin and stdout with /dev/null
			if (open("/dev/null", O_RDWR) != STDIN_FILENO)
				_exit(MAGIC_ERROR_STATUS);

			if (dup(STDIN_FILENO) != STDOUT_FILENO)
				_exit(MAGIC_ERROR_STATUS);

			// Replace stderr with pipe endpoint
			if (dup(stderr_pipe[1]) != STDERR_FILENO)
				_exit(MAGIC_ERROR_STATUS);
			close(stderr_pipe[1]);

			// Run lcoolc
			const std::string file_input = std::string(name) + ".cl";
			execl(info.lcoolc_path.c_str(), lcoolc_option, file_input.c_str(), (char*) NULL);
			_exit(MAGIC_ERROR_STATUS);
		}

		// Read contents of stderr and exit status
		close(stderr_pipe[1]);
		test_result stderr_contents = read_fd(stderr_pipe[0]);
		close(stderr_pipe[0]);

		int wstatus;
		if (waitpid(child, &wstatus, 0) < 0)
			return test_error_with_errno("waitpid: ");

		// Handle stderr read errors, now that we've reaped the child
		if (stderr_contents.status != test_status::pass)
			return stderr_contents;

		if (WIFEXITED(wstatus))
		{
			int expected_exit_status = (expected == build_expect::errors) ? 1 : 0;

			switch (WEXITSTATUS(wstatus))
			{
				case 0:
				case 1:
					// Check exit code and fail immediately if it's wrong
					if (WEXITSTATUS(wstatus) != expected_exit_status)
						break;

					// Compare with output file
					if (stderr_contents.err_msg != expected_output)
					{
						stderr_contents.status = test_status::fail;
						stderr_contents.err_msg += "== incorrect output - expected:\n";
						stderr_contents.err_msg += expected_output;
						return stderr_contents;
					}

					return { test_status::pass, "" };

				case MAGIC_ERROR_STATUS:
					// Child errored out in exec
					return { test_status::error, "exec failed (is the path to lcoolc correct?)" };
			}

			// Incorrect exit status
			stderr_contents.status = test_status::fail;
			stderr_contents.err_msg += "== exited with status " + std::to_string(WEXITSTATUS(wstatus));
			return stderr_contents;
		}
		else if (WIFSIGNALED(wstatus))
		{
			// Fatal signal
			stderr_contents.status = test_status::fail;

			const char* sig_string = strsignal(WTERMSIG(wstatus));
			if (sig_string == NULL)
				stderr_contents.err_msg += "== fatal signal " + std::to_string(WTERMSIG(wstatus));
			else
				stderr_contents.err_msg += "== fatal signal " + std::string(sig_string);

			return stderr_contents;
		}
		else
		{
			return { test_status::error, "unknown waitpid result" };
		}
	};
}

test_fptr lcool::test::parse(const char* name, build_expect expected)
{
	return build_only_test(name, expected, "--parse");
}

test_fptr lcool::test::compile(const char* name, build_expect expected)
{
	return build_only_test(name, expected, "-o-");
}

#if 0
test_fptr lcool::test::semantic(const char* file);
test_fptr lcool::test::semantic_input(const char* file);
#endif

// Runs a test and prints the results
static bool run_test(const std::pair<std::string, test_fptr>& test, const test_info& info)
{
	test_result result = test.second(info);

	if (result.status == test_status::pass)
	{
		std::cout << "PASS  " << test.first << std::endl;
		return true;
	}
	else
	{
		if (result.status == test_status::fail)
			std::cout << "FAIL  ";
		else
			std::cout << "ERROR ";

		std::cout << test.first << '\n';
		std::cout << "----------\n";
		std::cout << result.err_msg;
		if (result.err_msg.back() != '\n')
			std::cout << '\n';
		std::cout << "----------" << std::endl;
		return false;
	}
}

int main(int argc, char* argv[])
{
	const auto& tests = lcool::test::testcases;
	unsigned tests_passed = 0;

	// Get path to lcoolc from command line
	test_info info;
	if (argc != 2)
	{
		std::cerr << "usage: " << argv[0] << " <path to lcoolc>\n";
		return 1;
	}

	info.lcoolc_path = argv[1];

	// Run all the tests
	for (auto& test : tests)
	{
		if (run_test(test, info))
			tests_passed++;
	}

	// Print final line
	std::cout << '\n';
	std::cout << "Finished\n" << tests_passed << " out of " << tests.size()
		<< " passed" << std::endl;
	return (tests_passed == tests.size()) ? 0 : 1;
}
