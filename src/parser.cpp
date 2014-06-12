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
#include <utility>

#include "ast.hpp"
#include "lexer.hpp"
#include "logger.hpp"
#include "parser.hpp"
#include "smart_ptr.hpp"

namespace ast = lcool::ast;

using lcool::lexer;
using lcool::parse_error;
using lcool::token;
using lcool::token_type;

using lcool::make_shared;
using lcool::make_unique;
using lcool::shared_ptr;
using lcool::unique_ptr;

// ########################
// Parser Declarations
// ########################

namespace
{
	class parser
	{
	public:
		parser(std::istream& input, shared_ptr<const std::string>& filename, lcool::logger& log);
		ast::program parse();

	private:
		// Top-level parsers
		std::pair<std::string, ast::cls>            parse_class();
		void parse_feature(ast::cls& into);
		std::pair<std::string, ast::type_and_value> parse_attribute(token&& name);
		std::pair<std::string, ast::method>         parse_method(token&& name);

		ast::type_and_value parse_type_and_value();

		// Expression parsers
		unique_ptr<ast::expr> parse_expr();
		unique_ptr<ast::expr> parse_expr_not();
		unique_ptr<ast::expr> parse_expr_comp();
		unique_ptr<ast::expr> parse_expr_and();
		unique_ptr<ast::expr> parse_expr_mult();
		unique_ptr<ast::expr> parse_expr_isvoid();
		unique_ptr<ast::expr> parse_expr_negate();
		unique_ptr<ast::expr> parse_expr_dispatch();
		unique_ptr<ast::expr> parse_expr_base();

		// Insert a pair into a map
		template <typename T>
		void insert_symbol(
			std::map<std::string, T>& map, const std::string& map_name,
			std::pair<std::string, T>&& symbol, lcool::location loc)
		{
			// Add to list or error if the class already exists
			auto result = map.insert(std::move(symbol));
			if (!result.second)
			{
				const std::string& symbol_name = result.first->first;
				log.error(loc, "duplicate definition for " + map_name + " " + symbol_name);
			}
		}

		// Insert a pair into a map
		//  The type T must have a "loc" location attribute
		template <typename T>
		void insert_symbol(
			std::map<std::string, T>& map, const std::string& map_name,
			std::pair<std::string, T>&& symbol)
		{
			insert_symbol(map, map_name, std::move(symbol), symbol.second.loc);
		}

		// Construct an expression and moves the location in the lookahead token into it
		template <typename T>
		unique_ptr<T> make_expr()
		{
			auto result = make_unique<T>();
			result.loc = std::move(lookahead.loc);
			return result;
		}

		// Consume one token unconditionally or of the given type
		token consume();
		token consume(token_type type);

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
// Top-Level Parsers
// ########################

parser::parser(std::istream& input, shared_ptr<const std::string>& filename, lcool::logger& log)
	: log(log), my_lexer(input, filename)
{
	// Get first token
	consume();
}

ast::program parser::parse()
{
	ast::program result;

	// Consume all the classes
	while (lookahead.type == token_type::kw_class)
	{
		insert_symbol(result, "class", parse_class());
	}

	// Must end with EOF
	consume(token_type::eof);
	return result;
}

std::pair<std::string, ast::cls> parser::parse_class()
{
	std::string name;
	ast::cls result;

	// Extract class header
	result.loc = consume(token_type::kw_class).loc;
	name       = consume(token_type::type).value;
	if (optional(token_type::kw_inherits))
	{
		result.parent = consume(token_type::type).value;
	}

	// Extract features
	consume(token_type::lbraket);

	while (lookahead.type == token_type::id)
	{
		// Extract name and branch based on next symbol (aaarg 2 tokens of lookahead!!)
		token name = consume(token_type::id);

		if (lookahead.type == token_type::lparen)
		{
			insert_symbol(result.methods, "method", parse_method(std::move(name)));
		}
		else
		{
			insert_symbol(result.attributes, "attribute", parse_attribute(std::move(name)));
			consume(token_type::semicolon);
		}
	}

	consume(token_type::rbraket);
	consume(token_type::semicolon);

	return std::make_pair(std::move(name), std::move(result));
}

std::pair<std::string, ast::type_and_value> parser::parse_attribute(token&& name)
{
	ast::type_and_value result;

	consume(token_type::colon);

	// Extract location and type
	result.loc  = std::move(name.loc);
	result.type = consume(token_type::type).value;

	// Extract initial value
	if (optional(token_type::assign))
		result.initial = parse_expr();

	return std::make_pair(std::move(name.value), std::move(result));
}

std::pair<std::string, ast::method> parser::parse_method(token&& name)
{
	ast::method result;

	result.loc  = std::move(name.loc);
	consume(token_type::lparen);

	// Extract parameters
	do
	{
		// Handle first argument
		if (result.params.empty())
		{
			// Permit no arguments
			if (lookahead.type != token_type::id)
				break;
		}

		// Extract details and insert parameter
		token name_token = consume(token_type::id);
		consume(token_type::colon);
		token type_token = consume(token_type::type);

		insert_symbol(
			result.params,
			"parameter",
			std::make_pair(std::move(name.value), std::move(type_token.value)),
			name.loc);
	}
	while (optional(token_type::comma));

	consume(token_type::rparen);

	// Extract return type and body
	consume(token_type::colon);
	result.type = consume(token_type::type).value;
	consume(token_type::lbraket);
	result.body = parse_expr();
	consume(token_type::rbraket);

	return std::make_pair(std::move(name.value), std::move(result));
}

// ########################
// Expression Parsers
// ########################


/*
+			// Expressions
+			// TODO and, or, >, >= operators??
+			expr = *(term_id >> "<-") >> not_expr;
+
+			not_expr = *lit("not") >> comp_expr;
+
+			comp_expr = add_expr
+			    >> *("<=" >> add_expr |
+			          "<" >> add_expr |
+			          "=" >> add_expr);
+
+			add_expr = mult_expr
+			    >> *('+' >> mult_expr |
+			         '-' >> mult_expr);
+
+			mult_expr = isvoid_expr
+			    >> *('*' >> isvoid_expr |
+			         '/' >> isvoid_expr);
+
+			isvoid_expr = *lit("isvoid") >> negate_expr;
+
+			negate_expr = *lit('~') >> dispatch_expr;
+
+			dispatch_expr = base_expr
+			    >> *(-('@' >> term_type) >> '.' >> term_id >> '(' >> -(expr % ',') >> ')');
*/

unique_ptr<ast::expr> parser::parse_expr()
{
	return make_unique<ast::block>();
}

unique_ptr<ast::expr> parser::parse_expr_base()
{
	switch (lookahead.type)
	{
	case token_type::kw_if:
		{
			auto result = make_expr<ast::conditional>();

			consume(token_type::kw_if);
			result.predicate = parse_expr();
			consume(token_type::kw_then);
			result.if_true   = parse_expr();
			consume(token_type::kw_else);
			result.if_false  = parse_expr();
			consume(token_type::kw_fi);

			return std::move(result);
		}

	case token_type::kw_while:
		{
			auto result = make_expr<ast::loop>();

			consume(token_type::kw_while);
			result.predicate = parse_expr();
			consume(token_type::kw_loop);
			result.body      = parse_expr();
			consume(token_type::kw_pool);

			return std::move(result);
		}

	case token_type::lbraket:
		{
			auto result = make_expr<ast::block>();

			consume(token_type::lbraket);

			do
			{
				statements.emplace(parse_expr());
				consume(token_type::semicolon);
			}
			while (!optional(token_type::rbraket));

			return std::move(result);
		}

	case token_type::kw_let:
	case token_type::kw_case:
	case token_type::kw_new:
	case token_type::id:
	case token_type::integer:
	case token_type::string:
	case token_type::boolean:
	case token_type::lparen:
	default:
		// TODO Better messages?
		throw parse_error(lookahead.loc, "syntax error");
	}
/*
+			base_expr = "if" >> expr >> "then" >> expr >> "else" >> expr >> "fi"
+			          | "while" >> expr >> "loop" >> expr >> "pool"
+			          | '{' >> +(expr >> ';') >> '}'
+			          | "let" >> (attribute % ',') >> "in" >> expr
+			          | "case" >> expr >> "of" >> +(term_id >> ':' >> term_type >> "=>" >> expr >> ';') >> "esac"
+			          | "new" >> term_type
+			          | term_id >> '(' >> -(expr % ',') >> ')'
+			          | term_id
+			          | term_int
+			          | term_str
+			          | term_bool
+			          | '(' >> expr >> ')';
*/

	return make_unique<ast::block>();
}

// ########################
// Utility Methods
// ########################

token parser::consume()
{
	token result = std::move(lookahead);
	lookahead = my_lexer.scan_token();
	return result;
}

token parser::consume(token_type type)
{
	if (lookahead.type != type)
	{
		// TODO Error recovery?
		throw parse_error(lookahead.loc, "syntax error");
	}

	return consume();
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

ast::program lcool::parse(std::istream& input, const std::string& filename, lcool::logger& log)
{
	try
	{
		auto filename_shared = make_shared<const std::string>(filename);
		return parser(input, filename_shared, log).parse();
	}
	catch (parse_error& e)
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
