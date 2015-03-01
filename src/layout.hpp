/*
 * Copyright (C) 2015 James Cowgill
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

#ifndef LCOOL_LAYOUT_HPP
#define LCOOL_LAYOUT_HPP

#include "ast.hpp"
#include "cool_program.hpp"
#include "logger.hpp"

namespace lcool
{
	/**
	 * Inserts and lays out all the classes in an AST into a cool_program
	 *
	 * This function expects the standard builtin classes to already be added to the output.
	 *
	 * It is possible to call this multiple times on the same program, but earlier
	 * calls to this method must not reference classes defined in later calls (the
	 * first call would fail). It's probably better to just concatenate all the
	 * ASTs together and call this function once.
	 *
	 * If this method fails (see log.has_errors), the output program is left in
	 * an indeterminate state.
	 *
	 * @param input  AST to generate the program from
	 * @param output cool_program to write classes to
	 * @param log    logger to log any errors to
	 */
	void layout(const ast::program& input, cool_program& output, logger& log);
}

#endif
