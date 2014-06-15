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

#ifndef LCOOL_AST_HPP
#define LCOOL_AST_HPP

#include <boost/optional/optional.hpp>
#include <cstdint>
#include <string>
#include <vector>

#include "logger.hpp"
#include "smart_ptr.hpp"

namespace lcool { namespace ast
{
	class expr_visitor;

	/** Base class for expressions */
	class expr
	{
	public:
		virtual ~expr() { }

		/**
		 * Calls the relevant function of visitor depending on the type of this expression
		 *
		 * This method implements the visitor pattern / double dispatch for expressions.
		 * @param visitor class containing methods to call
		 */
		virtual void accept(expr_visitor& visitor) const = 0;

		/** The location of the start of this expression */
		location loc;
	};

	/** An attribute declaration (also used for let statements) */
	class attribute
	{
	public:
		/** Location of the attribute / variable declaration */
		location loc;

		/** Name of the attribute */
		std::string name;

		/** Type of variable / attribute */
		std::string type;

		/** Optional initial value */
		unique_ptr<expr> initial;
	};

	/** Expression assigning a value to an identifier */
	class assign : public expr
	{
	public:
		virtual void accept(expr_visitor& visitor) const;

		/** Identifier to assign to */
		std::string id;

		/** Value to assign */
		unique_ptr<expr> value;
	};

	/** Method dispatch / call expression */
	class dispatch : public expr
	{
	public:
		virtual void accept(expr_visitor& visitor) const;

		/** Name of method to call */
		std::string method_name;

		/** Optional object to call method on (or self) */
		unique_ptr<expr> object;

		/** Static type of the object being called */
		boost::optional<std::string> object_type;

		/** List of arguments */
		std::vector<unique_ptr<expr>> arguments;
	};

	/** Condition expression (if statement) */
	class conditional : public expr
	{
	public:
		virtual void accept(expr_visitor& visitor) const;

		/** Predicate to test on */
		unique_ptr<expr> predicate;

		/** Value to return if predicate is true */
		unique_ptr<expr> if_true;

		/** Value to return if predicate is false */
		unique_ptr<expr> if_false;
	};

	/** While loop */
	class loop : public expr
	{
	public:
		virtual void accept(expr_visitor& visitor) const;

		/** Predicate to test on */
		unique_ptr<expr> predicate;

		/** Body of the loop */
		unique_ptr<expr> body;
	};

	/** Statement block */
	class block : public expr
	{
	public:
		virtual void accept(expr_visitor& visitor) const;

		/** List of statements, last statement is the value of the block */
		std::vector<unique_ptr<expr>> statements;
	};

	/** Let expression (declares local variables + scope) */
	class let : public expr
	{
	public:
		virtual void accept(expr_visitor& visitor) const;

		/** List of variables to declare */
		std::vector<attribute> vars;

		/** Let expression body */
		unique_ptr<expr> body;
	};

	/** An individual branch of a type case expression */
	class type_case_branch
	{
	public:
		/** Name of the identifier to introduce with the more specific type */
		std::string id;

		/** Type to test for */
		std::string type;

		/** Body of the branch */
		unique_ptr<expr> body;
	};

	/** Type case expression (boo hiss) */
	class type_case : public expr
	{
	public:
		virtual void accept(expr_visitor& visitor) const;

		/** Value to test type of */
		unique_ptr<expr> value;

		/** List of case branches */
		std::vector<type_case_branch> branches;
	};

	/** Creates a new object of the given type */
	class new_object : public expr
	{
	public:
		virtual void accept(expr_visitor& visitor) const;

		/** Type of the new object */
		std::string type;
	};

	/** Constant boolean */
	class constant_bool : public expr
	{
	public:
		virtual void accept(expr_visitor& visitor) const;

		/** Value of the constant */
		bool value = false;
	};

	/** Constant integer */
	class constant_int : public expr
	{
	public:
		virtual void accept(expr_visitor& visitor) const;

		/** Value of the constant */
		std::int32_t value = 0;
	};

	/** Constant string */
	class constant_string : public expr
	{
	public:
		virtual void accept(expr_visitor& visitor) const;

		/** Value of the constant (processed to remove escape codes) */
		std::string value;
	};

	/** Read an identifier (local var / local attribute) */
	class identifier : public expr
	{
	public:
		virtual void accept(expr_visitor& visitor) const;

		/** Identifier to read */
		std::string id;
	};

	/** Types of unary operations */
	enum class compute_unary_type
	{
		isvoid,         /**< Tests if the expression is void */
		negate,         /**< Negates an integer expression */
		logical_not,    /**< Negates a boolean expression */
	};

	/** Computes some unary operation */
	class compute_unary : public expr
	{
	public:
		virtual void accept(expr_visitor& visitor) const;

		/** Type of expression */
		compute_unary_type op;

		/** Sub expression */
		unique_ptr<expr> body;
	};

	/** Types of binary operations */
	enum compute_binary_type
	{
		add,                /**< Adds two integers */
		subtract,           /**< Subtracts two integers */
		multiply,           /**< Multiplies two integers */
		divide,             /**< Divides two integers */
		less,               /**< Less than comparision */
		less_or_equal,      /**< Less than or equal comparision */
		equal,              /**< Value equality */
	};

	/** Computes some binary operation */
	class compute_binary : public expr
	{
	public:
		virtual void accept(expr_visitor& visitor) const;

		/** Type of expression */
		compute_binary_type op;

		/** Left sub expression */
		unique_ptr<expr> left;

		/** Right sub expression */
		unique_ptr<expr> right;
	};

	/** Visitor class used to traverse expression trees */
	class expr_visitor
	{
	public:
		virtual ~expr_visitor() { }

		virtual void visit(const assign&) = 0;
		virtual void visit(const dispatch&) = 0;
		virtual void visit(const conditional&) = 0;
		virtual void visit(const loop&) = 0;
		virtual void visit(const block&) = 0;
		virtual void visit(const let&) = 0;
		virtual void visit(const type_case&) = 0;
		virtual void visit(const new_object&) = 0;
		virtual void visit(const constant_bool&) = 0;
		virtual void visit(const constant_int&) = 0;
		virtual void visit(const constant_string&) = 0;
		virtual void visit(const identifier&) = 0;
		virtual void visit(const compute_unary&) = 0;
		virtual void visit(const compute_binary&) = 0;
	};

	/** AST for cool methods */
	class method
	{
	public:
		/** Method location */
		location loc;

		/** Method name */
		std::string name;

		/** Return type */
		std::string type;

		/** Method parameters */
		std::vector<std::pair<std::string, std::string>> params;

		/** Method body */
		unique_ptr<expr> body;
	};

	/** AST for a cool class */
	class cls
	{
	public:
		// The compiler isn't able to work out that this class
		//  cannot be copy constructed, so help it out a bit
		cls()                 = default;
		cls(cls&&)            = default;
		cls& operator=(cls&&) = default;

		/** Class location */
		location loc;

		/** Name of class */
		std::string name;

		/** Parent of the class (inherits from) */
		boost::optional<std::string> parent;

		/** Attribute definitions */
		std::vector<attribute> attributes;

		/** Method definitions */
		std::vector<method> methods;
	};

	/** Collection of classes which make up a program */
	typedef std::vector<cls> program;
}}

#endif
