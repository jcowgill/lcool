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
#include <stdexcept>
#include <string>

namespace lcool
{
	// The output from a pipeline
	struct pipeline_result
	{
		// Exit status for each process (as given by waitpid)
		std::vector<int> exit_statuses;

		// Output buffer contents
		std::vector<std::string> output_buffers;
	};

	// A pipeline is a series of executables with interconnected pipes, run
	// together
	class pipeline
	{
	public:
		// Adds a new process to the pipeline and returns its id
		//  Processes are executed + waited on in order of being added
		int add_process(std::vector<std::string> args);

		// Attaches a read/write /dev/null descriptor to a given fd
		//  (note this is the default for standard descriptors)
		void fd_null(int pid, int fd);

		// Attches an input file (rdonly) to an fd
		void fd_input_file(int pid, int fd, std::string filename);

		// Attaches an output buffer (wronly) to the given process's fd
		// Output buffers read in order of being attached
		int fd_output_buffer_new(int pid, int fd);

		// Attaches an existing output buffer to an fd
		void fd_output_buffer_dup(int pid, int fd, int bufid);

		// Attaches a pipe between processes in the pipeline
		void fd_pipe(int write_pid, int write_fd, int read_pid, int read_fd);

		// Executes the pipeline
		pipeline_result run() const;

	private:
		std::vector<std::vector<std::string>> _processes;
		std::vector<std::string> _input_files;

		int _num_output_buffers = 0;

	};
}
