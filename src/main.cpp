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
#include "logger.hpp"
#include "parser.hpp"

int main()
{
	// Parse and dump to stdout
	lcool::logger_ostream log;
	lcool::ast::program program = lcool::parse(std::cin, "stdin", log);

	if (log.has_errors())
		return 1;

	lcool::dump_ast(std::cout, program);
	return 0;
}
