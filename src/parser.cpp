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
		ast::cls       parse_class();
		ast::attribute parse_attribute();
		ast::method    parse_method();

		// Expression parsers
		unique_ptr<ast::expr> parse_expr();
		unique_ptr<ast::expr> parse_expr_not();
		unique_ptr<ast::expr> parse_expr_comp();
		unique_ptr<ast::expr> parse_expr_add();
		unique_ptr<ast::expr> parse_expr_mult();
		unique_ptr<ast::expr> parse_expr_isvoid();
		unique_ptr<ast::expr> parse_expr_negate();
		unique_ptr<ast::expr> parse_expr_dispatch();
		unique_ptr<ast::expr> parse_expr_base();

		// Literal parsers
		unique_ptr<ast::expr> parse_boolean();
		unique_ptr<ast::expr> parse_integer();
		unique_ptr<ast::expr> parse_string();

		// Construct an expression and moves the location in the lookahead token into it
		template <typename T>
		unique_ptr<T> make_expr()
		{
			auto result = make_unique<T>();
			result->loc = std::move(lookahead.loc);
			return result;
		}

		// Parse an infix expression
		unique_ptr<ast::expr> parse_infix(
			unique_ptr<ast::expr> (parser::*next_level)(),
			token_type token1, ast::compute_binary_type type1,
			token_type token2, ast::compute_binary_type type2,
			token_type token3, ast::compute_binary_type type3);

		// Consume one token unconditionally or of the given type
		token consume();
		token consume(token_type type);

		// Optionally consumes one token based on type
		bool optional(token_type type);

		// Reference to logger
		lcool::logger& log;

		// Lexer and two lookahead tokens
		lexer my_lexer;
		token lookahead, lookahead2;
	};
}

// ########################
// Top-Level Parsers
// ########################

parser::parser(std::istream& input, shared_ptr<const std::string>& filename, lcool::logger& log)
	: log(log), my_lexer(input, filename)
{
	// Get first 2 tokens
	consume();
	consume();
}

ast::program parser::parse()
{
	ast::program result;

	// Consume all the classes
	while (lookahead.type == token_type::kw_class)
	{
		result.push_back(parse_class());
	}

	// Must end with EOF
	consume(token_type::eof);
	return result;
}

ast::cls parser::parse_class()
{
	ast::cls result;

	// Extract class header
	result.loc  = consume(token_type::kw_class).loc;
	result.name = consume(token_type::type).value;
	if (optional(token_type::kw_inherits))
	{
		result.parent = consume(token_type::type).value;
	}

	// Extract features
	consume(token_type::lbraket);

	while (lookahead.type == token_type::id)
	{
		if (lookahead2.type == token_type::lparen)
		{
			result.methods.push_back(parse_method());
		}
		else
		{
			result.attributes.push_back(parse_attribute());
			consume(token_type::semicolon);
		}
	}

	consume(token_type::rbraket);
	consume(token_type::semicolon);

	return result;
}

ast::attribute parser::parse_attribute()
{
	ast::attribute result;

	token name = consume(token_type::id);
	consume(token_type::colon);

	// Extract location and type
	result.loc  = std::move(name.loc);
	result.name = std::move(name.value);
	result.type = consume(token_type::type).value;

	// Extract initial value
	if (optional(token_type::assign))
		result.initial = parse_expr();

	return result;
}

ast::method parser::parse_method()
{
	ast::method result;

	token name = consume(token_type::id);

	result.loc  = std::move(name.loc);
	result.name = std::move(name.value);
	consume(token_type::lparen);

	// Extract parameters
	do
	{
		// Handle first argument
		if (result.params.empty())
		{
			// Permit no arguments
			if (lookahead.type == token_type::rparen)
				break;
		}

		// Extract details and insert parameter
		token name_token = consume(token_type::id);
		consume(token_type::colon);
		token type_token = consume(token_type::type);

		result.params.push_back(
			std::make_pair(std::move(name.value), std::move(type_token.value)));
	}
	while (optional(token_type::comma));

	consume(token_type::rparen);

	// Extract return type and body
	consume(token_type::colon);
	result.type = consume(token_type::type).value;
	consume(token_type::lbraket);
	result.body = parse_expr();
	consume(token_type::rbraket);

	return result;
}

// ########################
// Expression Parsers
// ########################

unique_ptr<ast::expr> parser::parse_expr()
{
	// Detect assignment
	if (lookahead.type == token_type::id && lookahead2.type == token_type::assign)
	{
		auto result = make_expr<ast::assign>();

		result->id    = consume(token_type::id).value;
		consume(token_type::assign);
		result->value = parse_expr();

		return std::move(result);
	}

	return parse_expr_not();
}

unique_ptr<ast::expr> parser::parse_expr_not()
{
	// Detect not
	if (lookahead.type == token_type::kw_not)
	{
		auto result = make_expr<ast::compute_unary>();

		consume(token_type::kw_not);
		result->op   = ast::compute_unary_type::logical_not;
		result->body = parse_expr_not();

		return std::move(result);
	}

	return parse_expr_comp();
}

unique_ptr<ast::expr> parser::parse_expr_comp()
{
	auto left = parse_expr_add();

	while (lookahead.type == token_type::less_equal ||
	       lookahead.type == token_type::less ||
	       lookahead.type == token_type::equal)
	{
		auto new_left = make_expr<ast::compute_binary>();

		// Set token type
		switch (consume().type)
		{
			case token_type::less_equal: new_left->op = ast::compute_binary_type::less_or_equal;
			case token_type::less:       new_left->op = ast::compute_binary_type::less;
			case token_type::equal:      new_left->op = ast::compute_binary_type::equal;
		}

		// Move expressions
		new_left->left  = std::move(left);
		new_left->right = parse_expr_add();
		left            = std::move(new_left);
	}

	return left;
}

unique_ptr<ast::expr> parser::parse_expr_add()
{
	auto left = parse_expr_mult();

	while (lookahead.type == token_type::plus ||
	       lookahead.type == token_type::minus)
	{
		auto new_left = make_expr<ast::compute_binary>();

		// Set token type
		switch (consume().type)
		{
			case token_type::plus:  new_left->op = ast::compute_binary_type::add;
			case token_type::minus: new_left->op = ast::compute_binary_type::subtract;
		}

		// Move expressions
		new_left->left  = std::move(left);
		new_left->right = parse_expr_mult();
		left            = std::move(new_left);
	}

	return left;
}

unique_ptr<ast::expr> parser::parse_expr_mult()
{
	auto left = parse_expr_isvoid();

	while (lookahead.type == token_type::times ||
	       lookahead.type == token_type::divide)
	{
		auto new_left = make_expr<ast::compute_binary>();

		// Set token type
		switch (consume().type)
		{
			case token_type::times:  new_left->op = ast::compute_binary_type::multiply;
			case token_type::divide: new_left->op = ast::compute_binary_type::divide;
		}

		// Move expressions
		new_left->left  = std::move(left);
		new_left->right = parse_expr_isvoid();
		left            = std::move(new_left);
	}

	return left;
}

unique_ptr<ast::expr> parser::parse_expr_isvoid()
{
	// Detect isvoid
	if (lookahead.type == token_type::kw_isvoid)
	{
		auto result = make_expr<ast::compute_unary>();

		consume(token_type::kw_isvoid);
		result->op   = ast::compute_unary_type::isvoid;
		result->body = parse_expr_isvoid();

		return std::move(result);
	}

	return parse_expr_negate();
}

unique_ptr<ast::expr> parser::parse_expr_negate()
{
	// Detect negate
	if (lookahead.type == token_type::negate)
	{
		auto result = make_expr<ast::compute_unary>();

		consume(token_type::negate);
		result->op   = ast::compute_unary_type::negate;
		result->body = parse_expr_negate();

		return std::move(result);
	}

	return parse_expr_dispatch();
}

unique_ptr<ast::expr> parser::parse_expr_dispatch()
{
	auto left = parse_expr_base();

	while (lookahead.type == token_type::at ||
	       lookahead.type == token_type::dot)
	{
		auto new_left = make_expr<ast::dispatch>();

		// Extract object type
		if (optional(token_type::at))
			new_left->object_type = consume(token_type::type).value;

		consume(token_type::dot);

		// Extract method name and arguments
		new_left->method_name = consume(token_type::id).value;
		consume(token_type::lparen);

		do
		{
			// Handle first argument
			if (new_left->arguments.empty())
			{
				// Permit no arguments
				if (lookahead.type == token_type::rparen)
					break;
			}

			// Append argument
			new_left->arguments.push_back(parse_expr());
		}
		while (optional(token_type::comma));

		consume(token_type::rparen);

		// Finish method call
		new_left->object = std::move(left);
		left             = std::move(new_left);
	}

	return left;
}

unique_ptr<ast::expr> parser::parse_expr_base()
{
	switch (lookahead.type)
	{
	case token_type::kw_if:
		{
			auto result = make_expr<ast::conditional>();

			consume(token_type::kw_if);
			result->predicate = parse_expr();
			consume(token_type::kw_then);
			result->if_true   = parse_expr();
			consume(token_type::kw_else);
			result->if_false  = parse_expr();
			consume(token_type::kw_fi);

			return std::move(result);
		}

	case token_type::kw_while:
		{
			auto result = make_expr<ast::loop>();

			consume(token_type::kw_while);
			result->predicate = parse_expr();
			consume(token_type::kw_loop);
			result->body      = parse_expr();
			consume(token_type::kw_pool);

			return std::move(result);
		}

	case token_type::lbraket:
		{
			auto result = make_expr<ast::block>();

			consume(token_type::lbraket);

			do
			{
				result->statements.push_back(parse_expr());
				consume(token_type::semicolon);
			}
			while (!optional(token_type::rbraket));

			return std::move(result);
		}

	case token_type::kw_let:
#warning Does this need to have low precedence instead??
		{
			auto result = make_expr<ast::let>();

			consume(token_type::kw_let);

			do
			{
				result->vars.push_back(parse_attribute());
			}
			while (optional(token_type::comma));

			consume(token_type::kw_in);
			result->body = parse_expr();

			return std::move(result);
		}

	case token_type::kw_case:
		{
			auto result = make_expr<ast::type_case>();

			consume(token_type::kw_case);
			result->value = parse_expr();
			consume(token_type::kw_of);

			do
			{
				ast::type_case_branch branch;

				branch.id = consume(token_type::id).value;
				consume(token_type::colon);
				branch.type = consume(token_type::type).value;
				consume(token_type::case_arrow);
				branch.body = parse_expr();
				consume(token_type::semicolon);

				result->branches.push_back(std::move(branch));
			}
			while (!optional(token_type::kw_esac));

			return std::move(result);
		}

	case token_type::kw_new:
		{
			auto result = make_expr<ast::new_object>();

			consume(token_type::kw_new);
			result->type = consume(token_type::type).value;

			return std::move(result);
		}

	case token_type::id:
		{
#warning Do we need to do dispatch here??
			auto result = make_expr<ast::identifier>();
			result->id = consume(token_type::id).value;
			return std::move(result);
		}

	case token_type::integer:
		return parse_integer();

	case token_type::string:
		return parse_string();

	case token_type::boolean:
		return parse_boolean();

	case token_type::lparen:
		{
			consume(token_type::lparen);
			auto result = parse_expr();
			consume(token_type::rparen);
			return result;
		}

	default:
		// TODO Better messages?
		throw parse_error(lookahead.loc, "syntax error");
	}
}

unique_ptr<ast::expr> parser::parse_boolean()
{
	auto result = make_expr<ast::constant_bool>();
	result->value = (consume(token_type::boolean).value == "true");
	return std::move(result);
}

unique_ptr<ast::expr> parser::parse_integer()
{
	auto result = make_expr<ast::constant_int>();
	std::string str_value = consume(token_type::integer).value;
	std::uint32_t int_value = 0;

	// Convert string to integer
	for (char c : str_value)
	{
		// Shift and add to number
		int_value = (int_value << 1) + (c - '0');

		// Check for overflow
		if (int_value & (1 << 31))
		{
			log.warning(result->loc, "number cannot be represented: " + str_value);
			break;
		}
	}

	result->value = int_value;
	return std::move(result);
}

unique_ptr<ast::expr> parser::parse_string()
{
	auto result = make_expr<ast::constant_string>();
	std::string raw_value = consume(token_type::integer).value;
	bool escaped = false;

	for (char c : raw_value)
	{
		if (escaped)
		{
			// Escaped characters are copied verbatim except for some special ones
			switch (c)
			{
				case 'b': c = '\b'; break;
				case 'f': c = '\f'; break;
				case 'n': c = '\n'; break;
				case 'r': c = '\r'; break;
				case 't': c = '\t'; break;
			}

			result->value += c;
			escaped = false;
		}
		else if (c == '\\')
		{
			escaped = true;
		}
		else if (c != '"')
		{
			// Ignore unescaped quotes (start or end of string)
			result->value += c;
		}
	}

	return std::move(result);
}

// ########################
// Utility Methods
// ########################

token parser::consume()
{
	token result = std::move(lookahead);
	lookahead = std::move(lookahead2);
	lookahead2 = my_lexer.scan_token();
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
