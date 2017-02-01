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

#include <boost/range/combine.hpp>
#include <boost/format.hpp>

#include "codegen.hpp"

using namespace lcool;

namespace
{
// Creates a "dummy" ast::identifier node
ast::identifier create_dummy_identifier(std::string id)
{
	ast::identifier result;
	result.id = id;
	return result;
}

// Returns a pointer to the given slot in an object
llvm::Value* get_slot_pointer(
	llvm::IRBuilder<>& builder, llvm::Value* object, unsigned slot)
{
	std::vector<llvm::Value*> gep_args =
		{ builder.getInt32(0), builder.getInt32(slot) };
	return builder.CreateInBoundsGEP(object, gep_args);
}

// Loads an attribute (must be exact type)
llvm::Value* load_attribute(
	llvm::IRBuilder<>& builder, llvm::Value* object, cool_attribute* attr)
{
	return builder.CreateLoad(get_slot_pointer(builder, object, attr->struct_index));
}

// Stores a value into an attribute (must be exact type, no refcount handling)
void store_attribute(
	llvm::IRBuilder<>& builder, llvm::Value* object,
	cool_attribute* attr, llvm::Value* to_store)
{
	builder.CreateStore(to_store, get_slot_pointer(builder, object, attr->struct_index));
}

// Returns an object's default initialization value
llvm::Value* default_initializer(llvm::IRBuilder<>& builder, cool_class* cls)
{
	// For Ints, Bools and Strings, we call the constructor
	//  otherwise we return null
	if (cls->name() == "Int" || cls->name() == "Bool" || cls->name() == "String")
		return cls->create_object(builder);
	else
		return llvm::Constant::getNullValue(cls->llvm_type());
}

// Finds the first common parent class of the given types
cool_class* type_join(cool_class* a, cool_class* b)
{
	while (a != nullptr)
	{
		// If b is a subclass of a, return it
		// Otherwise go to a's parent. Eventually we'll get to Object
		//  which b must be a subtype of
		if (b->is_subclass_of(a))
			return a;

		a = a->parent();
	}

	// Should be impossible
	assert(false);
}

// An LLVM value attached to its cool class
struct value_and_cls
{
	llvm::Value* value;
	cool_class* cls;
};

// Stores the stack of arguments and local variables
class local_vars_stack
{
public:
	// Pushes a new variable
	//  info contains a POINTER to the variable instead of it's value
	void push(std::string name, value_and_cls info)
	{
		_vars.emplace_back(name, info);
	}

	// Pop n arguments off the stack
	void pop(unsigned n = 1)
	{
		assert(n <= _vars.size());
		_vars.resize(_vars.size() - n);
	}

	// Looks up a variable in the stack or returns boost::none if it
	//  doesn't exist
	boost::optional<value_and_cls> get(std::string name)
	{
		for (auto it = _vars.rbegin(); it != _vars.rend(); it++)
		{
			if (it->first == name)
				return it->second;
		}

		return boost::none;
	}

	// Number of locals
	unsigned size() const
	{
		return _vars.size();
	}

private:
	std::vector<std::pair<std::string, value_and_cls>> _vars;
};

// Class which handles code generation for each type of expression
class expr_codegen : public ast::expr_visitor
{
private:
	logger& _log;

	// What are we generating code for?
	cool_program& _program;
	cool_class* _declaring_cls;
	llvm::Function* _func;

	// Caches for common classes / objects
	cool_class* _builtin_int;
	cool_class* _builtin_bool;
	cool_class* _builtin_string;
	value_and_cls _zero;
	value_and_cls _null_object;

	// The IRBuilder also stores the block were writing instructions into
	llvm::IRBuilder<> _builder;

	// Holds the result of the most recent "visit" call
	value_and_cls _result = { nullptr, nullptr };

	// Local variables and arguments stack
	local_vars_stack _locals;

	// Dummy self node
	const ast::identifier _ast_self = create_dummy_identifier("self");

public:
	// Initializes the expression code generator from a class and llvm function
	expr_codegen(cool_program& program, cool_class* cls, llvm::Function* func, logger& log)
		: _log(log),
		  _program(program),
		  _declaring_cls(cls),
		  _func(func),
		  _builder(program.module()->getContext())
	{
		// Cache common cool types
		_builtin_int = program.lookup_class("Int");
		_builtin_bool = program.lookup_class("Bool");
		_builtin_string = program.lookup_class("String");

		// Cache zero and the null object
		_zero.value = _builder.getInt32(0);
		_zero.cls = _builtin_int;

		_null_object.cls = program.lookup_class("Object");
		_null_object.value = llvm::Constant::getNullValue(_null_object.cls->llvm_type());
	}


	// Initializes the expression code generator for the given method
	expr_codegen(cool_program& program, cool_method* method, logger& log)
		: expr_codegen(program, method->declaring_class(), method->llvm_func(), log)
	{
	}

	expr_codegen(const expr_codegen&) = delete;
	expr_codegen& operator=(const expr_codegen&) = delete;

	// Adds an argument to the list of local variables
	//  info.value contains a POINTER to the argument (not the value itself)
	bool add_argument(std::string name, value_and_cls info)
	{
		// Fail if an argument with that name exists
		if (_locals.get(name))
			return false;

		_locals.push(name, info);
		return true;
	}

	// Evaluates the given expression and returns the result
	//  The expression is evaluated at the current insert point and
	//   may have side effects. If the evaluation failes, an error is reported
	//   and a "fake" value is returned (but it can still be used).
	//  Code is written to the "current" block. This is usually the block given
	//   by the last call to evaluate(expr, block) but might be different if
	//   more blocks are created by the expression itself (eg for conditionals).
	value_and_cls evaluate(const ast::expr& expr)
	{
		expr.accept(*this);
		return _result;
	}

	// Evaluates an expression and writes the code starting at the end of the given block
	value_and_cls evaluate(const ast::expr& expr, llvm::BasicBlock* block)
	{
		_builder.SetInsertPoint(block);
		return evaluate(expr);
	}

	// Gets the "current" insert block (see evaluate)
	llvm::BasicBlock* get_insert_block()
	{
		return _builder.GetInsertBlock();
	}

	void visit(const ast::assign& expr) override
	{
#warning Handle refcounting
		// Evaluate subexpr and assign to _result immediately
		_result = evaluate(*expr.value);

		// Do the assignment
		if (expr.id == "self")
		{
			_log.error(expr.loc, "cannot assign to self");
			return;
		}

		auto local_info = _locals.get(expr.id);
		if (local_info)
		{
			// Store into local variable / argument
			auto coerced = _result.cls->upcast_to(_builder, _result.value, local_info->cls);
			if (coerced == nullptr)
			{
				_log.error(expr.loc, boost::format("invalid conversion from '%s' to '%s'") %
					_result.cls->name() % local_info->cls->name());
			}
			else
			{
				_builder.CreateStore(coerced, local_info->value);
			}
			return;
		}

		auto attr = _declaring_cls->lookup_attribute(expr.id);
		if (attr != nullptr)
		{
			// Store attribute
			auto coerced = _result.cls->upcast_to(_builder, _result.value, attr->type);
			if (coerced == nullptr)
			{
				_log.error(expr.loc, boost::format("invalid conversion from '%s' to '%s'") %
					_result.cls->name() % attr->type->name());
			}
			else
			{
				store_attribute(_builder, evaluate(_ast_self).value, attr, coerced);
			}
			return;
		}

		_log.error(expr.loc, "variable not defined '" + expr.id + "'");
	}

	void visit(const ast::dispatch& expr) override
	{
		// Evaluate all expressions which are part of the dispatch
		value_and_cls object;
		std::vector<value_and_cls> args;

		if (expr.object)
			object = evaluate(*expr.object);
		else
			object = evaluate(_ast_self);

		for (auto& arg_ptr : expr.arguments)
			args.push_back(evaluate(*arg_ptr));

		// Get class to dispatch against
		cool_class* cls = object.cls;
		bool force_static = false;
		if (expr.object_type)
		{
			force_static = true;
			cls = _program.lookup_class(*expr.object_type);
			if (cls == nullptr)
			{
				_log.error(expr.loc, "class not defined '" + *expr.object_type + "'");
				_result = _zero;
				return;
			}

			if (!object.cls->is_subclass_of(cls))
			{
				_log.error(expr.loc, "'" + object.cls->name() + "' to the left of the dispatch is not a subclass of '@" + cls->name() + "'");
				_result = _zero;
				return;
			}
		}

		// Find method
		cool_method* to_call = cls->lookup_method(expr.method_name, true);
		if (to_call == nullptr)
		{
			_log.error(expr.loc, "method '" + expr.method_name + "' not defined for class '" + cls->name() + "'");
			_result = _zero;
			return;
		}

		// Check number of args
		std::vector<cool_class*>& parameter_types = to_call->slot()->parameter_types;
		if (args.size() != parameter_types.size())
		{
			_log.error(expr.loc, boost::format("wrong number of arguments for method '%s.%s' (expected %u, got %u)") %
				cls->name() % to_call->slot()->name % parameter_types.size() % args.size());
			_result = _zero;
			return;
		}

		// Coerce all the objects to the right types
		std::vector<llvm::Value*> func_args;
		llvm::Value* coerced;

		coerced = object.cls->upcast_to(_builder, object.value, to_call->slot()->declaring_class);
		if (coerced == nullptr)
		{
			_log.error(expr.loc, boost::format("invalid conversion from '%s' to '%s'") %
				object.cls->name() % cls->name());
		}
		else
		{
			func_args.push_back(coerced);
		}

		for (auto arg_zip : boost::combine(args, parameter_types))
		{
			value_and_cls& arg = arg_zip.get<0>();
			cool_class* param_type = arg_zip.get<1>();
			coerced = arg.cls->upcast_to(_builder, arg.value, param_type);

			if (coerced == nullptr)
			{
				_log.error(expr.loc, boost::format("invalid conversion from '%s' to '%s'") %
					arg.cls->name() % param_type->name());
			}
			else
			{
				func_args.push_back(coerced);
			}
		}

		// Do the function call
		if (args.size() == parameter_types.size())
		{
			cls->ensure_not_null(_builder, coerced);
			_result.value = to_call->call(_builder, func_args, force_static);
			_result.cls = to_call->slot()->return_type;
		}
		else
		{
			_result = _zero;
		}
	}

	void visit(const ast::conditional& expr) override
	{
		// Evaluate predicate
		auto predicate = evaluate(*expr.predicate);

		if (predicate.cls != _builtin_bool)
		{
			_log.error(expr.loc, "conditional predicate must be a Bool");

			// Fake a boolean expression so we can continue to check for errors
			predicate.value = _builder.getFalse();
			predicate.cls = _builtin_bool;
		}

		// Create 3 more blocks for each part
		llvm::LLVMContext& context = _program.module()->getContext();
		auto block_true = llvm::BasicBlock::Create(context, "if_true", _func);
		auto block_false = llvm::BasicBlock::Create(context, "if_false", _func);
		auto block_done = llvm::BasicBlock::Create(context, "if_done", _func);

		// Insert the conditional jump
		_builder.CreateCondBr(predicate.value, block_true, block_false);

		// Evaluate both sides + record the final blocks
		auto result_true = evaluate(*expr.if_true, block_true);
		block_true = _builder.GetInsertBlock();
		auto result_false = evaluate(*expr.if_false, block_false);
		block_false = _builder.GetInsertBlock();

		// After evaluation, we need to coerce both results into the type
		//  common to both, then jump to the done block
		cool_class* common_type = type_join(result_true.cls, result_false.cls);
		_builder.SetInsertPoint(block_true);
		auto value_true = result_true.cls->upcast_to(_builder, result_true.value, common_type);
		_builder.CreateBr(block_done);

		_builder.SetInsertPoint(block_false);
		auto value_false = result_false.cls->upcast_to(_builder, result_false.value, common_type);
		_builder.CreateBr(block_done);

		// Create the final phi node in the done block
		_builder.SetInsertPoint(block_done);
		auto phi = _builder.CreatePHI(common_type->llvm_type(), 2);
		phi->addIncoming(value_true, block_true);
		phi->addIncoming(value_false, block_false);
		_result.value = phi;
		_result.cls = common_type;
	}

	void visit(const ast::loop& expr) override
	{
		// Create blocks for the predicate, loop body, and done block
		llvm::LLVMContext& context = _program.module()->getContext();
		auto block_predicate = llvm::BasicBlock::Create(context, "loop_predicate", _func);
		auto block_body = llvm::BasicBlock::Create(context, "loop_body", _func);
		auto block_done = llvm::BasicBlock::Create(context, "loop_done", _func);

		// Jump to the predicate immediately
		_builder.CreateBr(block_predicate);

		// Generate predicate code
		auto value_predicate = evaluate(*expr.predicate);
		if (value_predicate.cls != _builtin_bool)
		{
			_log.error(expr.loc, "loop predicate must be a Bool");

			// Fake a boolean expression so we can continue to check for errors
			value_predicate.value = _builder.getFalse();
			value_predicate.cls = _builtin_bool;
		}

		_builder.CreateCondBr(value_predicate.value, block_body, block_done);

		// Generate body code
#warning Handle refcounting (discarding objects)
		evaluate(*expr.body, block_body);
		_builder.CreateBr(block_predicate);

		// Result of a loop is always a void Object
		_builder.SetInsertPoint(block_done);
		_result = _null_object;
	}

	void visit(const ast::block& expr) override
	{
#warning Handle refcounting (discarding objects)
		// Evaluate each expression in sucession, keeping only the last result
		assert(!expr.statements.empty());
		for (auto& expr_ptr : expr.statements)
			_result = evaluate(*expr_ptr);
	}

	void visit(const ast::let& expr) override
	{
		// Fetch init block
		auto init_block = &_func->front();
		assert(init_block->getName().equals("init"));

		for (const ast::attribute& var : expr.vars)
		{
			// Evaluate initializer (if there is one)
			value_and_cls initializer;
			if (var.initial)
				initializer = evaluate(*var.initial);

			// Check for "self" name
			if (var.name == "self")
			{
				_log.error(var.loc, "illegal variable name 'self'");
				continue;
			}

			// Lookup class
			cool_class* cls = _program.lookup_class(var.type);
			if (cls == nullptr)
			{
				_log.error(var.loc, "class not defined '" + var.type + "'");
				continue;
			}

			// Generate default initializer if there isn't one
			if (!var.initial)
			{
				initializer.value = default_initializer(_builder, cls);
			}
			else
			{
				// Coerce type so it can be stored
				initializer.value = initializer.cls->upcast_to(_builder, initializer.value, cls);
				if (initializer.value == nullptr)
				{
					_log.error(expr.loc, boost::format("invalid conversion from '%s' to '%s'") %
						initializer.cls->name() % cls->name());
				}
			}

			// Allocate some memory for this variable
			auto saved_block = _builder.GetInsertBlock();
			_builder.SetInsertPoint(init_block);

			value_and_cls pointer_value;
			pointer_value.value = _builder.CreateAlloca(cls->llvm_type(), nullptr, var.name);
			pointer_value.cls = cls;
			_builder.SetInsertPoint(saved_block);

			// Store value
			_builder.CreateStore(initializer.value, pointer_value.value);

			// Push variable onto stack
			_locals.push(var.name, pointer_value);
		}

		// Evaluate body
		_result = evaluate(*expr.body);

#warning Handle refcounting (discarding objects)
		// Pop variables gone out of scope
		_locals.pop(expr.vars.size());
	}

	void visit(const ast::type_case& expr) override
	{
#warning Implement case
		_log.error(expr.loc, "case not implemented");
		_result = _zero;
	}

	void visit(const ast::new_object& expr) override
	{
		// Lookup class and create object of that type
		cool_class* cls = _program.lookup_class(expr.type);
		if (cls == nullptr)
		{
			_log.error(expr.loc, "class not defined '" + expr.type + "'");
			_result = _null_object;
		}
		else
		{
			_result.value = cls->create_object(_builder);
			_result.cls = cls;
		}
	}

	void visit(const ast::constant_bool& expr) override
	{
		if (expr.value)
			_result.value = _builder.getTrue();
		else
			_result.value = _builder.getFalse();

		_result.cls = _builtin_bool;
	}

	void visit(const ast::constant_int& expr) override
	{
		_result.value = _builder.getInt32(expr.value);
		_result.cls = _builtin_int;
	}

	void visit(const ast::constant_string& expr) override
	{
#warning Handle refcounting?
		_result.value = _program.create_string_literal(expr.value);
		_result.cls = _builtin_string;
	}

	void visit(const ast::identifier& expr) override
	{
#warning Handle refcounting?

		auto local_info = _locals.get(expr.id);
		if (local_info)
		{
			// Load local variable / argument
			_result.value = _builder.CreateLoad(local_info->value);
			_result.cls = local_info->cls;
			return;
		}

		auto attr = _declaring_cls->lookup_attribute(expr.id);
		if (attr != nullptr)
		{
			// Load attribute
			_result.value = load_attribute(_builder, evaluate(_ast_self).value, attr);
			_result.cls = attr->type;
			return;
		}

		// Identifier not found!
		_log.error(expr.loc, "variable not defined '" + expr.id + "'");
		_result = _zero;
	}

	void visit(const ast::compute_unary& expr) override
	{
		llvm::PointerType* subexpr_ptr_type;
		auto subexpr = evaluate(*expr.body);

		switch (expr.op)
		{
			case ast::compute_unary_type::isvoid:
				// Don't bother with anything for ints / bools
				subexpr_ptr_type = llvm::dyn_cast<llvm::PointerType>(subexpr.value->getType());
				if (subexpr_ptr_type == nullptr)
				{
					_log.warning(expr.loc, "isvoid on Int or Bool is always false");
					_result.value = _builder.getFalse();
				}
				else
				{
#warning Handle refcounting?
					// Test for null
					auto const_null = llvm::ConstantPointerNull::get(subexpr_ptr_type);
					_result.value = _builder.CreateICmpEQ(subexpr.value, const_null);
				}

				_result.cls = _builtin_bool;
				break;

			case ast::compute_unary_type::negate:
				// Integer negation
				if (subexpr.cls != _builtin_int)
				{
					_log.error(expr.loc, "input to ~ operator must be an Int");
					_result = _zero;
				}
				else
				{
					_result.value = _builder.CreateNeg(subexpr.value);
					_result.cls = _builtin_int;
				}
				break;

			case ast::compute_unary_type::logical_not:
				// Boolean complement
				if (subexpr.cls != _builtin_bool)
				{
					_log.error(expr.loc, "input to 'not' operator must be a Bool");
					_result.value = _builder.getFalse();
				}
				else
				{
					_result.value = _builder.CreateNot(subexpr.value);
				}

				_result.cls = _builtin_bool;
				break;

			default:
				assert(0);
		}
	}

	void visit(const ast::compute_binary& expr) override
	{
		auto left = evaluate(*expr.left);
		auto right = evaluate(*expr.right);

		// Equality is special as it accepts many different types
		if (expr.op == ast::compute_binary_type::equal)
		{
			_result.cls = _builtin_bool;

			if ((left.cls == _builtin_bool) != (right.cls == _builtin_bool)
				|| (left.cls == _builtin_int) != (right.cls == _builtin_int)
				|| (left.cls == _builtin_string) != (right.cls == _builtin_string))
			{
				_log.error(expr.loc, "basic types can only be compared with themselves");
				_result.value = _builder.getFalse();
			}
			else if (left.cls == _builtin_string)
			{
				// String equality
				auto to_call = _program.module()->getFunction("String$equals");
				auto call_inst = _builder.CreateCall(to_call, { left.value, right.value });
				call_inst->setCallingConv(to_call->getCallingConv());
				_result.value = call_inst;
			}
			else if(left.cls->is_subclass_of(right.cls) || right.cls->is_subclass_of(left.cls))
			{
				// Upcast one side to the other
				if (left.cls->is_subclass_of(right.cls))
					left.value = left.cls->upcast_to(_builder, left.value, right.cls);
				else
					right.value = right.cls->upcast_to(_builder, right.value, left.cls);

				// Eveyrthing else compares pointers / values for equality
				_result.value = _builder.CreateICmpEQ(left.value, right.value);
			}
			else
			{
				// If the above condition failes, the types can never be equal
				_log.warning(expr.loc, "result of comparison is always false");
				_result.value = _builder.getFalse();
			}
		}
		else if (expr.op == ast::compute_binary_type::add ||
				expr.op == ast::compute_binary_type::subtract ||
				expr.op == ast::compute_binary_type::multiply ||
				expr.op == ast::compute_binary_type::divide)
		{
			// Arithmetic expression - only accepts ints, result is always int
			_result = _zero;
			if (left.cls != _builtin_int || right.cls != _builtin_int)
			{
				_log.error(expr.loc, "both inputs to an arithmetic expression must be Ints");
			}
			else
			{
				switch (expr.op)
				{
					case ast::compute_binary_type::add:
						_result.value = _builder.CreateAdd(left.value, right.value);
						break;

					case ast::compute_binary_type::subtract:
						_result.value = _builder.CreateSub(left.value, right.value);
						break;

					case ast::compute_binary_type::multiply:
						_result.value = _builder.CreateMul(left.value, right.value);
						break;

					case ast::compute_binary_type::divide:
						// Check for division by zero
						_program.call_global(_builder, "zero_division_check", right.value);

						// Do the division
						_result.value = _builder.CreateSDiv(left.value, right.value);
						break;

					default:
						assert(0);
				}
			}
		}
		else
		{
			// Comparison expression - only accepts ints, result is always bool
			_result.cls = _builtin_bool;
			if (left.cls != _builtin_int || right.cls != _builtin_int)
			{
				_log.error(expr.loc, "both inputs to a comparison expression must be Ints");
				_result.value = _builder.getFalse();
			}
			else if (expr.op == ast::compute_binary_type::less)
			{
				_result.value = _builder.CreateICmpSLT(left.value, right.value);
			}
			else
			{
				_result.value = _builder.CreateICmpSLE(left.value, right.value);
			}
		}
	}
};

// Generates the copy constructor
void gen_copy_constructor(cool_program& output, cool_class* cls)
{
	llvm::Module* module = output.module();
	llvm::LLVMContext& context = module->getContext();

	// Get llvm function and argument
	llvm::Function* func = cls->copy_constructor();
	assert(func->empty());

	// Create builder
	llvm::IRBuilder<> builder(context);
	builder.SetInsertPoint(llvm::BasicBlock::Create(context, "", func));

	// Get this and other pointers
	llvm::Value* pthis_obj = &func->getArgumentList().front();
	llvm::Value* other_obj = &*(func->getArgumentList().begin()++);
	llvm::Value* pthis = cls->downcast(builder, pthis_obj);
	llvm::Value* other = cls->downcast(builder, other_obj);

	// Call parent copy constructor
	llvm::Function* parent_func = cls->parent()->copy_constructor();
	auto call_inst = builder.CreateCall(parent_func, { pthis_obj, other_obj });
	call_inst->setCallingConv(llvm::CallingConv::Fast);

	// Copy each attribute
	for (cool_attribute* attr : cls->attributes())
	{
		auto value = load_attribute(builder, other, attr);
		attr->type->refcount_inc(builder, value);
		store_attribute(builder, pthis, attr, value);
	}

	builder.CreateRetVoid();
}

// Generates the destructor for the given class
void gen_destructor(cool_program& output, cool_class* cls)
{
	llvm::Module* module = output.module();
	llvm::LLVMContext& context = module->getContext();

	// Get llvm function and argument
	llvm::Function* func = cls->destructor();
	assert(func->empty());

	// Create builder
	llvm::IRBuilder<> builder(context);
	builder.SetInsertPoint(llvm::BasicBlock::Create(context, "", func));

	// Get object to destroy and downcast it
	llvm::Value* to_destroy_obj = &func->getArgumentList().front();
	llvm::Value* to_destroy = cls->downcast(builder, to_destroy_obj);

	// Destroy each attribute
	for (cool_attribute* attr : cls->attributes())
		attr->type->refcount_dec(builder, load_attribute(builder, to_destroy, attr));

	// Call parent destructor
	llvm::Function* parent_func = cls->parent()->destructor();
	auto call_inst = builder.CreateCall(parent_func, to_destroy_obj);
	call_inst->setCallingConv(llvm::CallingConv::Fast);
	call_inst->setTailCall();
	builder.CreateRetVoid();
}

// Generates a class's constructor
void gen_constructor(const ast::cls& input, cool_program& output, cool_class* cls, logger& log)
{
#warning Remove code duplication between gen_constructor and gen_method
	// Create a code generator
	llvm::Function* func = cls->constructor();
	expr_codegen the_generator(output, cls, func, log);

	// Create the init and user blocks
	llvm::LLVMContext& context = output.module()->getContext();
	auto init_block = llvm::BasicBlock::Create(context, "init", func);
	auto user_block = llvm::BasicBlock::Create(context, "", func);
	llvm::IRBuilder<> builder(init_block);

	// Add self (which might be used in an initializer)
	llvm::Value* self_ptr = builder.CreateAlloca(cls->llvm_type());
	llvm::Value* self = builder.CreateBitCast(&func->getArgumentList().front(), cls->llvm_type());
	builder.CreateStore(self, self_ptr);
	the_generator.add_argument("self", { self_ptr, cls });

	// Call parent constructor
	llvm::Value* raw_object = &func->getArgumentList().front();
	builder.SetInsertPoint(user_block);
	auto call_inst = builder.CreateCall(cls->parent()->constructor(), raw_object);
	call_inst->setCallingConv(llvm::CallingConv::Fast);

	// Default initialize all attributes
	for (auto attr : cls->attributes())
		store_attribute(builder, self, attr, default_initializer(builder, attr->type));

	// Call each attribute's initializer (if it exists)
#warning refcounting?
	for (auto& ast_attr : input.attributes)
	{
		if (ast_attr.initial)
		{
			auto result = the_generator.evaluate(*ast_attr.initial, builder.GetInsertBlock());
			builder.SetInsertPoint(the_generator.get_insert_block());

			// Coerce result to attribute's type
			auto attr = cls->lookup_attribute(ast_attr.name);
			auto upcasted = result.cls->upcast_to(builder, result.value, attr->type);
			if (upcasted == nullptr)
			{
				log.error(ast_attr.loc, boost::format("invalid conversion from '%s' to '%s'") %
					result.cls->name() % attr->type->name());
			}
			else
			{
				// Store result
				store_attribute(builder, self, attr, upcasted);
			}
		}
	}

	// Do the final stitchup
	builder.CreateRetVoid();
	builder.SetInsertPoint(init_block);
	builder.CreateBr(user_block);
}

// Extracts a vector containing the arguments of a function
std::vector<llvm::Value*> get_func_arguments(llvm::Function* func)
{
	std::vector<llvm::Value*> result;
	for (auto& arg : func->args())
		result.push_back(&arg);
	return result;
}

// Generates code for the given method
void gen_method(const ast::method& input, cool_program& output, cool_class* cls, logger& log)
{
	// Lookup method
	cool_method* method = cls->lookup_method(input.name);
	assert(method != nullptr);

	// Create a code generator
	expr_codegen the_generator(output, method, log);

	// Create the init block which will hold self, args and locals
	llvm::LLVMContext& context = output.module()->getContext();
	llvm::Function* func = method->llvm_func();
	std::vector<llvm::Value*> func_args = get_func_arguments(func);

	auto init_block = llvm::BasicBlock::Create(context, "init", func);
	llvm::IRBuilder<> builder(init_block);

	// Add self
#warning how does refcounting work with these vars
	assert(!func_args.empty());
	llvm::Value* self_ptr = builder.CreateAlloca(cls->llvm_type());
	llvm::Value* self = method->slot()->declaring_class->downcast(builder, func_args[0]);
	builder.CreateStore(self, self_ptr);
	the_generator.add_argument("self", { self_ptr, cls });

	// Add all arguments
	for (unsigned i = 0; i < input.params.size(); i++)
	{
		cool_class* arg_cls = method->slot()->parameter_types[i];
		llvm::Value* arg_ptr = builder.CreateAlloca(arg_cls->llvm_type());
		llvm::Value* arg = func_args[i + 1];
		builder.CreateStore(arg, arg_ptr);
		the_generator.add_argument(input.params[i].first, { arg_ptr, arg_cls });
	}

	// Generate the main code body
	auto user_block = llvm::BasicBlock::Create(context, "", func);
	auto result = the_generator.evaluate(*input.body, user_block);

	// Coerce the result to the correct return type
#warning Refcounting?
	builder.SetInsertPoint(the_generator.get_insert_block());
	auto upcasted = result.cls->upcast_to(builder, result.value, method->slot()->return_type);
	if (upcasted == nullptr)
	{
		log.error(input.loc, boost::format("returning: invalid conversion from '%s' to '%s'") %
			result.cls->name() % method->slot()->return_type->name());
	}
	else
	{
		// Do the final return
		builder.CreateRet(upcasted);
	}

	// Finally, insert a branch from the init block to the first user block
	builder.SetInsertPoint(init_block);
	builder.CreateBr(user_block);
}

// Generates the code for a given class
void codegen_cls(const ast::cls& input, cool_program& output, logger& log)
{
	cool_class* cls = output.lookup_class(input.name);
	assert(cls != nullptr);

	// Generate simple functions (copy constructor + destructor)
	gen_copy_constructor(output, cls);
	gen_destructor(output, cls);

	// Generate constructor
	gen_constructor(input, output, cls, log);

	// Generata all methods
	for (auto& method : input.methods)
		gen_method(method, output, cls, log);
}

void gen_main_func(cool_program& output, logger& log)
{
	llvm::Module* module = output.module();
	llvm::LLVMContext& context = output.module()->getContext();

	auto func_type = llvm::FunctionType::get(llvm::Type::getInt32Ty(context), false);
	llvm::Function* func = llvm::Function::Create(
		func_type,
		llvm::Function::ExternalLinkage,
		"main",
		module);
	llvm::BasicBlock* block = llvm::BasicBlock::Create(context, "", func);

	// Create main class
	llvm::IRBuilder<> builder(block);
	cool_class* main_cls = output.lookup_class("Main");
	if (main_cls == nullptr)
	{
		log.error("'Main' class not defined");
		return;
	}

	llvm::Value* main_obj = main_cls->create_object(builder);

	// Call main function
	cool_method* main_obj_func = main_cls->lookup_method("main", true);
	if (main_obj_func == nullptr)
	{
		log.error("method 'Main.main' not defined");
		return;
	}

	if (!main_obj_func->slot()->parameter_types.empty())
	{
		log.error("method 'Main.main' must have no parameters");
		return;
	}

	// We can handle this case fine, so it's just a warning
	if (main_obj_func->declaring_class() != main_cls)
	{
		log.warning("method 'Main.main' not declared in 'Main' class");
		main_obj = main_cls->upcast_to(builder, main_obj, main_obj_func->declaring_class());
	}

	auto return_value = main_obj_func->call(builder, { main_obj }, true);

	// Decrement refcounts
	main_obj_func->slot()->return_type->refcount_dec(builder, return_value);
	main_cls->refcount_dec(builder, main_obj);

	// Return
	builder.CreateRet(builder.getInt32(0));
}

}

void lcool::codegen(const ast::program& input, cool_program& output, logger& log)
{
	// Generate code for every class
	for (auto& cls : input)
		codegen_cls(cls, output, log);

	// Create main function
	gen_main_func(output, log);
}
