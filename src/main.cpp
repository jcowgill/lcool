/*
 * Copyright (C) 2014 James Cowgill
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

#include <iostream>

#include "ast.hpp"
#include "builtins.hpp"
#include "cool_program.hpp"
#include "layout.hpp"
#include "logger.hpp"
#include "parser.hpp"

int main()
{
	// Parse and dump to stdout
	lcool::logger_ostream log;
	lcool::ast::program program = lcool::parse(std::cin, "stdin", log);

	if (log.has_errors())
		return 1;

	// Dump AST
	lcool::dump_ast(std::cout, program);

	// Create empty cool_program
	llvm::LLVMContext llvm_context;
	lcool::cool_program output(llvm_context);

	// Link in builtin classes
	lcool::load_builtins(output);

	// Layout program
	lcool::layout(program, output, log);

	if (log.has_errors())
		return 1;

	return 0;
}
