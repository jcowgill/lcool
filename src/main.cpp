/*
 * Copyright (C) 2014-2015 James Cowgill
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

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_os_ostream.h>
#include <fstream>
#include <iostream>

#include "ast.hpp"
#include "builtins.hpp"
#include "codegen.hpp"
#include "cool_program.hpp"
#include "layout.hpp"
#include "logger.hpp"
#include "parser.hpp"

#define LCOOL_VERSION "0.1"

namespace po = boost::program_options;

int main(int argc, char* argv[])
{
	po::options_description visible("Allowed options");
	visible.add_options()
		("help,h", "print help message")
		("version", "print version")
		("parse", "dump the parse tree instead of doing a full compile")
		("output,o", po::value<std::string>(), "specify output file");

	po::options_description config("Hidden options");
	config.add(visible);
	config.add_options()
		("input", po::value<std::vector<std::string>>(), "specify input files");

	po::positional_options_description p;
	p.add("input", -1);

	po::variables_map vm;
	po::store(po::command_line_parser(argc, argv).options(config).positional(p).run(), vm);
	po::notify(vm);

	if (vm.count("version"))
	{
		std::cerr << "lcoolc version " << LCOOL_VERSION << std::endl;
		return 1;
	}

	if (vm.count("help"))
	{
		std::cerr << "lcoolc [options] input files..." << std::endl;
		std::cerr << " Compiles COOL sources into LLVM bitcode" << std::endl;
		std::cerr << visible << std::endl;
		return 1;
	}

	// Parse input files
	lcool::logger_ostream log;
	if (!vm.count("input"))
	{
		log.error("no input files");
		return 1;
	}

	auto inputs = vm["input"].as<std::vector<std::string>>();
	lcool::ast::program program;

	for (std::string filename : inputs)
	{
		lcool::ast::program parse_result;

		if (filename == "-")
		{
			parse_result = lcool::parse(std::cin, "stdin", log);
		}
		else
		{
			std::ifstream file(filename, std::ios::in | std::ios::binary);
			if (file.fail())
			{
				log.error(boost::format("error opening '%s': %s") % filename % std::strerror(errno));
				continue;
			}

			parse_result = lcool::parse(file, filename, log);
		}

		program.insert(
			program.end(),
			std::make_move_iterator(parse_result.begin()),
			std::make_move_iterator(parse_result.end()));
	}

	if (log.has_errors())
		return 1;

	// Dump parse tree if requested
	if (vm.count("parse"))
	{
		lcool::dump_ast(std::cout, program);
		return 0;
	}

	// Create empty cool_program
	llvm::LLVMContext llvm_context;
	lcool::cool_program output(llvm_context);

	// Link in builtin classes
	lcool::load_builtins(output);

	// Layout program
	lcool::layout(program, output, log);
	if (log.has_errors())
		return 1;

	// Generate code
	lcool::codegen(program, output, log);
	if (log.has_errors())
		return 1;

	// Verify module
	llvm::verifyModule(*output.module());

	// Get output filename
	std::string out_filename;
	if (vm.count("output"))
		out_filename = vm["output"].as<std::string>();
	else if (inputs[0] == "-")
		out_filename = "-";
	else if (boost::algorithm::ends_with(inputs[0], ".cl"))
		out_filename = inputs[0].substr(0, inputs[0].size() - 2) + "bc";
	else
		out_filename = inputs[0] + ".bc";

	// Write bitcode
	if (out_filename == "-")
	{
		llvm::raw_os_ostream stream(std::cout);
		llvm::WriteBitcodeToFile(output.module(), stream);
	}
	else
	{
		std::ofstream file(out_filename, std::ios::out | std::ios::binary | std::ios::trunc);
		if (file.fail())
		{
			log.error(boost::format("error opening '%s': %s") % out_filename % std::strerror(errno));
			return 1;
		}

		llvm::raw_os_ostream stream(file);
		llvm::WriteBitcodeToFile(output.module(), stream);
	}

	return 0;
}
