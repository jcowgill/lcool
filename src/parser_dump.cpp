/*
 * Copyright (C) 2014-2015 James Cowgill
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

#include <iostream>

#include "ast.hpp"
#include "parser.hpp"

namespace ast = lcool::ast;

namespace
{
	// Dumps an AST (or parts of it)
	class ast_dumper : public ast::expr_visitor
	{
	public:
		ast_dumper(std::ostream& out, int indent = 0);

		// Top-level dumpers
		void dump(const ast::program&);
		void dump(const ast::attribute&);
		void dump(const ast::method&);
		void dump(const ast::expr&);

		// Expression dumpers
		virtual void visit(const ast::assign&) override;
		virtual void visit(const ast::dispatch&) override;
		virtual void visit(const ast::conditional&) override;
		virtual void visit(const ast::loop&) override;
		virtual void visit(const ast::block&) override;
		virtual void visit(const ast::let&) override;
		virtual void visit(const ast::type_case&) override;
		virtual void visit(const ast::new_object&) override;
		virtual void visit(const ast::constant_bool&) override;
		virtual void visit(const ast::constant_int&) override;
		virtual void visit(const ast::constant_string&) override;
		virtual void visit(const ast::identifier&) override;
		virtual void visit(const ast::compute_unary&) override;
		virtual void visit(const ast::compute_binary&) override;

	private:
		// Prints the indent and returns the output stream
		std::ostream& print_indent();

		// Run dump on something while indenting it
		template <typename T>
		void dump_indented(const T& thing, int amount = 1)
		{
			indent_ += amount;
			dump(thing);
			indent_ -= amount;
		}

		// Current indent level and output stream
		std::ostream& out_;
		int indent_;
	};
}

ast_dumper::ast_dumper(std::ostream& out, int indent)
	:out_(out), indent_(indent)
{
}

void ast_dumper::dump(const ast::program& program)
{
	for (const ast::cls& cls : program)
	{
		// Dump class
		print_indent() << "class '" << cls.name << "' (" << cls.loc << ")\n";
		if (cls.parent)
			print_indent() << " inherits '" << *cls.parent << "'\n";

		// Dump attributes
		for (const ast::attribute& attribute : cls.attributes)
			dump_indented(attribute);

		// Dump methods
		for (const ast::method& method : cls.methods)
			dump_indented(method);
	}

	out_ << std::flush;
}

void ast_dumper::dump(const ast::attribute& attribute)
{
	print_indent() << "attribute '" << attribute.name << "' (" << attribute.loc << ")\n";
	print_indent() << " type '" << attribute.type << "'\n";

	if (attribute.initial)
	{
		print_indent() << " initial =\n";
		dump_indented(*attribute.initial, 2);
	}
}

void ast_dumper::dump(const ast::method& method)
{
	print_indent() << "method '" << method.name << "' (" << method.loc << ")\n";
	print_indent() << " returns '" << method.type << "'\n";

	if (method.params.empty())
	{
		print_indent() << " no params\n";
	}
	else
	{
		print_indent() << " params\n";

		for (const std::pair<std::string, std::string>& param : method.params)
		{
			print_indent() << "  '" << param.first << "' of type '" << param.second << "'\n";
		}
	}

	dump_indented(*method.body);
}

void ast_dumper::dump(const ast::expr& expr)
{
	expr.accept(*this);
}

void ast_dumper::visit(const ast::assign& e)
{
	print_indent() << "assign to '" << e.id << "' (" << e.loc << ")\n";
	dump_indented(*e.value);
}

void ast_dumper::visit(const ast::dispatch& e)
{
	print_indent() << "dispatch to method '" << e.method_name << "' (" << e.loc << ")\n";

	if (e.object)
	{
		print_indent() << " on\n";
		dump_indented(*e.object, 2);
	}
	else
	{
		print_indent() << " on self\n";
	}

	if (e.object_type)
		print_indent() << " via type '" << *e.object_type << "'\n";

	if (e.arguments.empty())
	{
		print_indent() << " no arguments\n";
	}
	else
	{
		print_indent() << " arguments\n";

		for (const lcool::unique_ptr<ast::expr>& arg : e.arguments)
			dump_indented(*arg, 2);
	}
}

void ast_dumper::visit(const ast::conditional& e)
{
	print_indent() << "conditional (" << e.loc << ")\n";
	print_indent() << " predicate\n";
	dump_indented(*e.predicate, 2);
	print_indent() << " if true\n";
	dump_indented(*e.if_true, 2);
	print_indent() << " if false\n";
	dump_indented(*e.if_false, 2);
}

void ast_dumper::visit(const ast::loop& e)
{
	print_indent() << "loop (" << e.loc << ")\n";
	print_indent() << " predicate\n";
	dump_indented(*e.predicate, 2);
	print_indent() << " body\n";
	dump_indented(*e.body, 2);
}

void ast_dumper::visit(const ast::block& e)
{
	print_indent() << "block (" << e.loc << ")\n";

	for (const lcool::unique_ptr<ast::expr>& statement : e.statements)
		dump_indented(*statement);
}

void ast_dumper::visit(const ast::let& e)
{
	print_indent() << "let (" << e.loc << ")\n";

	for (const ast::attribute& var : e.vars)
		dump_indented(var);

	print_indent() << " body\n";
	dump_indented(*e.body, 2);
}

void ast_dumper::visit(const ast::type_case& e)
{
	print_indent() << "case (" << e.loc << ")\n";
	print_indent() << " value\n";
	dump_indented(*e.value, 2);

	for (const ast::type_case_branch& branch : e.branches)
	{
		print_indent() << " branch '" << branch.type << "' with name '" << branch.id << "'\n";
		dump_indented(*branch.body, 2);
	}
}

void ast_dumper::visit(const ast::new_object& e)
{
	print_indent() << "new (" << e.loc << ")\n";
	print_indent() << " type '" << e.type << "'\n";
}

void ast_dumper::visit(const ast::constant_bool& e)
{
	const char * value_str = (e.value ? "true" : "false");

	print_indent() << "boolean " << value_str << " (" << e.loc << ")\n";
}

void ast_dumper::visit(const ast::constant_int& e)
{
	print_indent() << "integer " << e.value << " (" << e.loc << ")\n";
}

void ast_dumper::visit(const ast::constant_string& e)
{
	print_indent() << "string \"" << e.value << "\" (" << e.loc << ")\n";
}

void ast_dumper::visit(const ast::identifier& e)
{
	print_indent() << "identifier '" << e.id << "' (" << e.loc << ")\n";
}

void ast_dumper::visit(const ast::compute_unary& e)
{
	const char * expr_type = "!BAD UNARY!";

	switch (e.op)
	{
	case ast::compute_unary_type::isvoid:      expr_type = "isvoid"; break;
	case ast::compute_unary_type::negate:      expr_type = "negate"; break;
	case ast::compute_unary_type::logical_not: expr_type = "logical not"; break;
	}

	print_indent() << expr_type << " (" << e.loc << ")\n";
	dump_indented(*e.body);
}

void ast_dumper::visit(const ast::compute_binary& e)
{
	const char * expr_type = "!BAD BINARY!";

	switch (e.op)
	{
	case ast::compute_binary_type::add:              expr_type = "add"; break;
	case ast::compute_binary_type::subtract:         expr_type = "subtract"; break;
	case ast::compute_binary_type::multiply:         expr_type = "multiply"; break;
	case ast::compute_binary_type::divide:           expr_type = "divide"; break;
	case ast::compute_binary_type::less:             expr_type = "less than"; break;
	case ast::compute_binary_type::less_or_equal:    expr_type = "less than or equal"; break;
	case ast::compute_binary_type::equal:            expr_type = "equal"; break;
	}

	print_indent() << expr_type << " (" << e.loc << ")\n";
	dump_indented(*e.left);
	dump_indented(*e.right);
}

// Prints the indent and returns cout
std::ostream& ast_dumper::print_indent()
{
	for (int i = 0; i < indent_; i++)
		out_ << ' ';

	return out_;
}

void lcool::dump_ast(std::ostream& output, const ast::program& program)
{
	ast_dumper(output).dump(program);
}
