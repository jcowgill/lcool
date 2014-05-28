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
			start = qi::lit("1")[&blank_prog] >> qi::eoi;
		}

	private:
		rule<ast::program()> start;
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
