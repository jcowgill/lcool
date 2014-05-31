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

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/support_istream_iterator.hpp>

#include "ast.hpp"
#include "logger.hpp"
#include "parser.hpp"

namespace spirit = boost::spirit;
namespace qi = boost::spirit::qi;
namespace enc = boost::spirit::ascii;

namespace ast = lcool::ast;

namespace
{
	// Cool skipper (whitespace and comments)
	template <typename Iterator>
	class cool_skipper : public qi::grammar<Iterator>
	{
	public:
		cool_skipper()
			: cool_skipper::base_type(start)
		{
			// Skip spaces, line comments or multi-line comments
			start = enc::space
			      | single_comment
			      | multi_comment;

			// Rules for different comment styles
			single_comment = "--" >> *(enc::char_ - qi::eol);
			multi_comment  = "(*" >> *(multi_comment | (enc::char_ - "*)")) >> "*)";
		}

	private:
		qi::rule<Iterator> start, single_comment, multi_comment;
	};

	// The cool grammar object
	template <typename Iterator>
	class cool_grammar : public qi::grammar<Iterator, ast::program(), cool_skipper<Iterator>>
	{
	public:
		// qi rule type
		template <typename Result>
		using rule = qi::rule<Iterator, Result, cool_skipper<Iterator>>;

		static ast::program blank_prog()
		{
			return ast::program();
		}

		cool_grammar(const std::string& filename, lcool::logger& log)
			: cool_grammar::base_type(start)
		{
			using enc::char_;
			using qi::int_parser;
			using qi::lexeme;
			using qi::lit;
			using qi::string;

			// Terminals
			term_id   = lexeme[char_("A-Za-z_") >> *char_("A-Za-z0-9_")];
			term_type = term_id;
			term_int  = int_parser<int32_t>();
			term_bool = string("true") | string("false");
			term_str  = lexeme[char_('"')
			            >> *(char_('\\') >> char_ | (char_ - '\n' - '\\' - '"'))
			            >> char_('"')];

			// Start rule
			// TODO start = qi::lit("1")[&blank_prog] >> qi::eoi;
			start = *cls >> qi::eoi;

			// Classes
			// TODO Should the final semicolon be optional?
			cls = "class" >> term_type >> -("inherits" >> term_type)
			      >>'{' >> *feature >> '}' >> ';';

			// Features (method or attribute)
			// TODO Left factor this?
			// TODO Should the final semicolon be optional?
			feature   = method >> ';'
			          | attribute >> ';';

			method    = term_id >> '(' >> -((term_id >> ':' >> term_type) % ',') >> ')'
			            >> ':' >> term_type >> '{' >> expr >> '}';
			attribute = term_id >> ':' >> term_type >> -("<-" >> expr);

			// Expressions
			// TODO and, or, >, >= operators??
			expr = *(term_id >> "<-") >> not_expr;

			not_expr = *lit("not") >> comp_expr;

			comp_expr = add_expr
			    >> *("<=" >> add_expr |
			          "<" >> add_expr |
			          "=" >> add_expr);

			add_expr = mult_expr
			    >> *('+' >> mult_expr |
			         '-' >> mult_expr);

			mult_expr = isvoid_expr
			    >> *('*' >> isvoid_expr |
			         '/' >> isvoid_expr);

			isvoid_expr = *lit("isvoid") >> negate_expr;

			negate_expr = *lit('~') >> dispatch_expr;

			dispatch_expr = base_expr
			    >> *(-('@' >> term_type) >> '.' >> term_id >> '(' >> -(expr % ',') >> ')');

			base_expr = "if" >> expr >> "then" >> expr >> "else" >> expr >> "fi"
			          | "while" >> expr >> "loop" >> expr >> "pool"
			          | '{' >> +(expr >> ';') >> '}'
			          | "let" >> (attribute % ',') >> "in" >> expr
			          | "case" >> expr >> "of" >> +(term_id >> ':' >> term_type >> "=>" >> expr >> ';') >> "esac"
			          | "new" >> term_type
			          | term_id >> '(' >> -(expr % ',') >> ')'
			          | term_id
			          | term_int
			          | term_str
			          | term_bool
			          | '(' >> expr >> ')';
		}

	private:
		rule<ast::program()> start;

		using rulez = qi::rule<Iterator, cool_skipper<Iterator>>;
		rulez cls;
		rulez feature;
		rulez method;
		rulez attribute;

		rulez expr;
		rulez not_expr, comp_expr, add_expr, mult_expr, isvoid_expr, negate_expr, dispatch_expr, base_expr;

		// Terminals
		rulez term_id, term_type, term_int, term_bool, term_str;
	};
}

ast::program lcool::parse(
	std::istream& input,
	const std::string& filename,
	lcool::logger& log)
{
	// Disable whitespace skipping on input stream
	input.unsetf(std::ios::skipws);

	// Construct iterators
	spirit::istream_iterator iter(input);
	spirit::istream_iterator end;

	// Create grammar objects
	cool_grammar<spirit::istream_iterator> grammar(filename, log);
	cool_skipper<spirit::istream_iterator> skipper;

	// Parse it
	ast::program class_map;
	if (!qi::phrase_parse(iter, end, grammar, skipper, class_map))
	{
		// TODO Print something better here (iter points to failing point)
		log.error("parse error");
	}

	return class_map;
}

#include <iostream>

int main(void)
{
	lcool::logger_ostream log;
	lcool::parse(std::cin, "", log);
	return 0;
}
