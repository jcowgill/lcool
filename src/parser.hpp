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

#ifndef LCOOL_PARSER_HPP
#define LCOOL_PARSER_HPP

#include <istream>

#include "ast.hpp"
#include "logger.hpp"

namespace lcool
{
	/**
	 * Parses the given input file into the output program
	 *
	 * @param filename the name of the file being parsed
	 * @param input    the input stream containing the file data
	 * @param log      the logger to print any errors / warnings to
	 * @return the list of parsed classes
	 */
	ast::program parse(const std::string& filename, std::istream& input, logger& log);
}

#endif
