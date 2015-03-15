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

#ifndef LCOOL_CODEGEN_HPP
#define LCOOL_CODEGEN_HPP

#include "ast.hpp"
#include "cool_program.hpp"
#include "logger.hpp"

namespace lcool
{
	/**
	 * Generates the LLVM code for all the classes in the input program
	 *
	 * Before calling this, all the classes must be created layed out first.
	 *
	 * @param input program to read code from
	 * @param output program to write code to
	 * @param log logger to log errors to
	 */
	void codegen(const ast::program& input, cool_program& output, logger& log);
}

#endif
