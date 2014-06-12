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
		virtual void accept(expr_visitor& visitor) = 0;

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
		virtual void accept(expr_visitor& visitor);

		/** Identifier to assign to */
		std::string id;

		/** Value to assign */
		unique_ptr<expr> value;
	};

	/** Method dispatch / call expression */
	class dispatch : public expr
	{
	public:
		virtual void accept(expr_visitor& visitor);

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
		virtual void accept(expr_visitor& visitor);

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
		virtual void accept(expr_visitor& visitor);

		/** Predicate to test on */
		unique_ptr<expr> predicate;

		/** Body of the loop */
		unique_ptr<expr> body;
	};

	/** Statement block */
	class block : public expr
	{
	public:
		virtual void accept(expr_visitor& visitor);

		/** List of statements, last statement is the value of the block */
		std::vector<unique_ptr<expr>> statements;
	};

	/** Let expression (declares local variables + scope) */
	class let : public expr
	{
	public:
		virtual void accept(expr_visitor& visitor);

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
		virtual void accept(expr_visitor& visitor);

		/** Value to test type of */
		unique_ptr<expr> value;

		/** List of case branches */
		std::vector<type_case_branch> branches;
	};

	/** Creates a new object of the given type */
	class new_object : public expr
	{
	public:
		virtual void accept(expr_visitor& visitor);

		/** Type of the new object */
		std::string type;
	};

	/** Constant boolean */
	class constant_bool : public expr
	{
	public:
		virtual void accept(expr_visitor& visitor);

		/** Value of the constant */
		bool value = false;
	};

	/** Constant integer */
	class constant_int : public expr
	{
	public:
		virtual void accept(expr_visitor& visitor);

		/** Value of the constant */
		int32_t value = 0;
	};

	/** Constant string */
	class constant_string : public expr
	{
	public:
		virtual void accept(expr_visitor& visitor);

		/** Value of the constant (processed to remove escape codes) */
		std::string value;
	};

	/** Read an identifier (local var / local attribute) */
	class identifier : public expr
	{
	public:
		virtual void accept(expr_visitor& visitor);

		/** Identifier to read */
		std::string id;
	};

	/** Computes some unary operation */
	class compute_unary : public expr
	{
	public:
		virtual void accept(expr_visitor& visitor);

		/** Types of unary operations */
		enum op_type
		{
			ISVOID,     /**< Tests if the expression is void */
			NEGATE,     /**< Negates an integer expression */
			NOT         /**< Negates a boolean expression */
		};

		/** Type of expression */
		op_type op;

		/** Sub expression */
		unique_ptr<expr> body;
	};

	/** Computes some binary operation */
	class compute_binary : public expr
	{
	public:
		virtual void accept(expr_visitor& visitor);

		/** Types of unary operations */
		enum op_type
		{
			ADD,                /**< Adds two integers */
			SUBTRACT,           /**< Subtracts two integers */
			MULTIPLY,           /**< Multiplies two integers */
			DIVIDE,             /**< Divides two integers */
			LESS,               /**< Less than comparision */
			LESS_OR_EQUAL,      /**< Less than or equal comparision */
			GREATER,            /**< (extension) Greater than comparision */
			GREATER_OR_EQUAL,   /**< (extension) Greater than or equal comparision */
			EQUAL,              /**< Value equality */
			NOT_EQUAL           /**< (extension) Value inequality */
		};

		/** Type of expression */
		op_type op;

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

		virtual void visit(assign& e) = 0;
		virtual void visit(dispatch& e) = 0;
		virtual void visit(conditional& e) = 0;
		virtual void visit(loop& e) = 0;
		virtual void visit(block& e) = 0;
		virtual void visit(let& e) = 0;
		virtual void visit(type_case& e) = 0;
		virtual void visit(new_object& e) = 0;
		virtual void visit(constant_bool& e) = 0;
		virtual void visit(constant_int& e) = 0;
		virtual void visit(constant_string& e) = 0;
		virtual void visit(identifier& e) = 0;
		virtual void visit(compute_unary& e) = 0;
		virtual void visit(compute_binary& e) = 0;
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
