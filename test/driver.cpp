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

#include <cassert>
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
using lcool::test::test_error;
using lcool::test::test_fptr;
using lcool::test::test_info;
using lcool::test::test_result;
using lcool::test::test_status;

// If the child reports this exit code, there was an error during exec
#define MAGIC_ERROR_STATUS 125

// Retutns a test error with errno appended to it
static test_error test_error_with_errno(std::string prefix)
{
	return test_error(prefix + " (" + strerror(errno) + ")");
}

namespace
{
	// Class which holds 2 pipe file descriptors
	class pipe_fds
	{
	public:
		// Creates a new pipe
		pipe_fds()
		{
			if (pipe(_fds) != 0)
				throw test_error_with_errno("pipe: ");
		}

		pipe_fds(const pipe_fds&) = delete;
		pipe_fds& operator=(const pipe_fds&) = delete;
		~pipe_fds() { close(); }

		int read_fd() const { assert(_fds[0] >= 0); return _fds[0]; }
		int write_fd() const { assert(_fds[1] >= 0); return _fds[1]; }

		void close() noexcept { close_read(); close_write(); }

		void close_read() noexcept
		{
			if (_fds[0] >= 0)
			{
				::close(_fds[0]);
				_fds[0] = -1;
			}
		}

		void close_write() noexcept
		{
			if (_fds[1] >= 0)
			{
				::close(_fds[1]);
				_fds[1] = -1;
			}
		}

	private:
		int _fds[2];
	};

	// Class that ensures zombies are not left around when an error occurs
	class child_reaper
	{
	public:
		explicit child_reaper(pid_t child)
			: _child(child)
		{
		}

		child_reaper(const child_reaper&) = delete;
		child_reaper& operator=(const child_reaper&) = delete;
		~child_reaper() { wait(false); }

		int wait(bool throw_on_error = true)
		{
			// Only wait once
			pid_t child = _child;
			if (child < 0)
				return -1;
			_child = -1;

			int wstatus;
			if (waitpid(child, &wstatus, 0) < 0)
			{
				if (throw_on_error)
					throw test_error_with_errno("waitpid: ");
				else
					return -1;
			}

			return wstatus;
		}

	private:
		pid_t _child;
	};
}


// Reads contents of file desciptor into a string
static std::string read_fd(int fd)
{
	std::string result;
	char buffer[1024];
	ssize_t bytes_read;

	while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0)
		result.append(buffer, bytes_read);

	if (bytes_read < 0)
		throw test_error_with_errno("read: ");

	return result;
}

// Reads an entire file into a string
static std::string read_file(const std::string& filename)
{
	int fd = open(filename.c_str(), O_RDONLY);
	if (fd < 0)
		throw test_error_with_errno(std::string("open ") + filename + ": ");

	std::string result = read_fd(fd);
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
			expected_output = read_file(std::string(name) + ".out");

		// Fork lcoolc process
		pipe_fds stderr_pipe;
		pid_t child = fork();
		if (child == -1)
			throw test_error_with_errno("fork: ");

		if (child == 0)
		{
			// Close old standard file descriptors
			close(STDIN_FILENO);
			close(STDOUT_FILENO);
			close(STDERR_FILENO);

			// Replace stdin and stdout with /dev/null
			if (open("/dev/null", O_RDWR) != STDIN_FILENO)
				_exit(MAGIC_ERROR_STATUS);

			if (dup(STDIN_FILENO) != STDOUT_FILENO)
				_exit(MAGIC_ERROR_STATUS);

			// Replace stderr with pipe endpoint
			if (dup(stderr_pipe.write_fd()) != STDERR_FILENO)
				_exit(MAGIC_ERROR_STATUS);
			stderr_pipe.close();

			// Run lcoolc
			const std::string file_input = std::string(name) + ".cl";
			execl(info.lcoolc_path.c_str(), lcoolc_option, file_input.c_str(), (char*) NULL);
			_exit(MAGIC_ERROR_STATUS);
		}

		// Read contents of stderr
		child_reaper reaper(child);

		stderr_pipe.close_write();
		std::string stderr_contents = read_fd(stderr_pipe.read_fd());
		stderr_pipe.close();

		// Wait for child to exit and handle exit status
		int wstatus = reaper.wait();
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
					if (stderr_contents != expected_output)
					{
						stderr_contents += "== incorrect output - expected:\n";
						stderr_contents += expected_output;
						return { test_status::fail, stderr_contents };
					}

					return { test_status::pass, "" };

				case MAGIC_ERROR_STATUS:
					// Child errored out in exec
					throw test_error("exec failed (is the path to lcoolc correct?)");
			}

			// Incorrect exit status
			stderr_contents += "== exited with status " + std::to_string(WEXITSTATUS(wstatus));
			return { test_status::fail, stderr_contents };
		}
		else if (WIFSIGNALED(wstatus))
		{
			// Fatal signal
			const char* sig_string = strsignal(WTERMSIG(wstatus));
			if (sig_string == NULL)
				stderr_contents += "== fatal signal " + std::to_string(WTERMSIG(wstatus));
			else
				stderr_contents += "== fatal signal " + std::string(sig_string);

			return { test_status::fail, stderr_contents };
		}
		else
		{
			throw test_error("unknown waitpid result");
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
	test_result result;

	try
	{
		result = test.second(info);
	}
	catch (const test_error& e)
	{
		result = { test_status::error, e.what() };
	}

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
