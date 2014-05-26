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

#define BOOST_SPIRIT_UNICODE

#include <boost/spirit/include/qi.hpp>
//#include <boost/spirit/include/support.hpp>

#include "ast.hpp"
#include "logger.hpp"
#include "parser.hpp"

namespace qi = boost::spirit::qi;
namespace unicode = boost::spirit::unicode;

namespace ast = lcool::ast;

namespace
{
	// The cool grammar object
	template <typename Iterator>
	class cool_grammar : qi::grammar<Iterator, ast::program(), unicode::space_type>
	{
	};
}

ast::program parse(
	const std::string& filename,
	std::istream& input,
	lcool::logger& log)
{
	ast::program class_map;

	// utf8 to u32 needed

	return class_map;
}
