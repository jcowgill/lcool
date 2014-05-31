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
#include <stdexcept>

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
	// Types of token
	enum token_type
	{
		TOK_EOF,
	};

	// A token read by the lexer
	struct token
	{
		lcool::location loc;
		token_type      type;
		string          value;
	};

	// Thrown when a parse error occurs
	//  Nothing is written to the log yet when this is thrown
	class parse_error : public std::runtime_error
	{
	public:
		parse_error(const lcool::location& loc, const string& msg)
			: runtime_error(msg), loc(loc)
		{
		}

		const lcool::location loc;
	};

	class lexer
	{
	public:
		lexer(istream& input, shared_ptr<const string>& filename);

		// Scans the next token from the input stream
		//  Throws parse_error if a lexical error occurs
		token scan_token();

	private:
		istream& input;
		lcool::location loc;
	};

	class parser
	{
	public:
		parser(istream& input, shared_ptr<const string>& filename, lcool::logger& log);
		ast::program parse();

	private:
		// Consume one token unconditionally or of the given type
		void consume();
		void consume(token_type type);

		// Optionally consumes one token based on type
		bool optional(token_type type);

		// Reference to logger
		lcool::logger& log;

		// Lexer and single lookahead token
		lexer my_lexer;
		token lookahead;
	};
}

// ########################
// lexer
// ########################

lexer::lexer(istream& input, shared_ptr<const string>& filename)
	: input(input), loc { filename, 1, 1 }
{
}

token lexer::scan_token()
{
}

// ########################
// parser
// ########################

parser::parser(istream& input, shared_ptr<const string>& filename, lcool::logger& log)
	: log(log), my_lexer(input, filename)
{
}

ast::program parser::parse()
{
	return ast::program();
}

void parser::consume()
{
	lookahead = my_lexer.scan_token();
}

void parser::consume(token_type type)
{
	if (!optional(type))
	{
		// TODO Error recovery?
		throw parse_error(lookahead.loc, "syntax error");
	}
}

bool parser::optional(token_type type)
{
	if (lookahead.type == type)
	{
		consume();
		return true;
	}

	return false;
}

// ########################
// External Functions
// ########################

ast::program lcool::parse(istream& input, const string& filename, lcool::logger& log)
{
	try
	{
		auto filename_shared = make_shared<const string>(filename);
		return parser(input, filename_shared, log).parse();
	}
	catch(parse_error& e)
	{
		// Log error and die
		log.error(e.loc, e.what());
		return ast::program();
	}
}

#include <iostream>

int main(void)
{
	lcool::logger_ostream log;
	lcool::parse(std::cin, "", log);
	return 0;
}
