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

#ifndef LCOOL_LEXER_HPP
#define LCOOL_LEXER_HPP

#include <boost/format/format_fwd.hpp>
#include <cstdint>
#include <iosfwd>

#include "logger.hpp"
#include "smart_ptr.hpp"

namespace lcool
{
	/** Types of token */
	enum token_type
	{
		// Special tokens (TOK_COMMENT is discarded internally and never returned)
		TOK_EOF,
		TOK_COMMENT,

		// Variable tokens (can have differing values)
		TOK_ID,
		TOK_TYPE,
		TOK_INTEGER,
		TOK_STRING,
		TOK_BOOLEAN,

		// Symbols
		TOK_SEMICOLON,  // ;
		TOK_COLON,      // :
		TOK_COMMA,      // ,
		TOK_AT,         // @
		TOK_DOT,        // .
		TOK_LBRAKET,    // {
		TOK_RBRAKET,    // }
		TOK_LPAREN,     // (
		TOK_RPAREN,     // )
		TOK_PLUS,       // +
		TOK_MINUS,      // -
		TOK_TIMES,      // *
		TOK_DIVIDE,     // /
		TOK_NEGATE,     // ~
		TOK_LESS,       // <
		TOK_LESS_EQUAL, // <=
		TOK_EQUAL,      // =
		TOK_ASSIGN,     // <-
		TOK_CASE_ARROW, // =>

		// Keywords
		TOK_CASE,
		TOK_CLASS,
		TOK_ELSE,
		TOK_ESAC,
		TOK_FI,
		TOK_IF,
		TOK_IN,
		TOK_INHERITS,
		TOK_ISVOID,
		TOK_LET,
		TOK_LOOP,
		TOK_NEW,
		TOK_NOT,
		TOK_OF,
		TOK_POOL,
		TOK_THEN,
		TOK_WHILE,
	};

	/** A token read by the lexer */
	struct token
	{
		lcool::location loc;
		token_type      type;
		std::string     value;
	};

	/**
	 * Thrown when a parse error occurs
	 *  Nothing is written to the log yet when this is thrown
	 */
	class parse_error : public std::runtime_error
	{
	public:
		parse_error(const lcool::location& loc, const std::string& msg);
		parse_error(const lcool::location& loc, const boost::format& msg);

		const lcool::location loc;
	};

	/** Class which generates a stream of tokens from a character stream */
	class lexer
	{
	public:
		lexer(std::istream& input, shared_ptr<const std::string>& filename);

		/**
		 * Scans the next token from the input stream
		 *  Throws parse_error if a lexical error occurs
		 */
		token scan_token();

	private:
		std::istream& input;
		location loc;
		int lookahead;

		// Scan a token without discarding comments
		token scan_token_all();

		// Special terminal parsers
		void parse_identifier(token& into);
		void parse_string(token& into);
		void parse_integer(token& into);
		void parse_comment_single();
		void parse_comment_multi();

		/** Consumes one character of input (returns the value in lookahead) */
		int consume_char();

		/** Consumes one character and appends it to a token's value */
		int consume_char(token& into);
	};
}

#endif
