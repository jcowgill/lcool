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

#include <istream>
#include <memory>
#include <string>

#include "ast.hpp"
#include "logger.hpp"
#include "parser.hpp"
#include "smart_ptr.hpp"

namespace ast = lcool::ast;

using std::istream;
using std::string;

using lcool::make_shared;
using lcool::make_unique;
using lcool::shared_ptr;
using lcool::unique_ptr;

// ########################
// Parser Declarations
// ########################

namespace
{
	class parser_base
	{
	protected:
		parser_base(istream& input, const string& filename, lcool::logger& log);

		lcool::logger& log;

	private:
		istream& input;
		shared_ptr<const string> filename;
	};

	class parser : public parser_base
	{
	public:
		parser(istream& input, const string& filename, lcool::logger& log);
		ast::program parse();

	private:
	};
}

// ########################
// parser_base
// ########################

parser_base::parser_base(istream& input, const string& filename, lcool::logger& log)
	: log(log), input(input), filename(make_shared<const string>(filename))
{
	// Disable whitespace skipping on input stream
	input.unsetf(std::ios::skipws);
}

// ########################
// parser
// ########################

parser::parser(istream& input, const string& filename, lcool::logger& log)
	: parser_base(input, filename, log)
{
}

ast::program parser::parse()
{
	return ast::program();
}

// ########################
// External Functions
// ########################

ast::program lcool::parse(istream& input, const string& filename, lcool::logger& log)
{
	return parser(input, filename, log).parse();
}

#include <iostream>

int main(void)
{
	lcool::logger_ostream log;
	lcool::parse(std::cin, "", log);
	return 0;
}
