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

#include <cassert>
#include <set>

#include "ast.hpp"
#include "builtins.hpp"
#include "cool_program.hpp"

using namespace lcool;

// ========= cool_method =========================================

lcool::cool_method::cool_method(std::unique_ptr<cool_method_slot> slot)
{
	_slot = slot.release();
	_declaring_class = _slot->declaring_class;
}

lcool::cool_method::cool_method(cool_class* declaring_class, cool_method* base_method)
	: _slot(base_method->slot()), _declaring_class(declaring_class)
{
}

lcool::cool_method::~cool_method()
{
	// Delete slot if non-null and we own it
	if (_slot != nullptr && _slot->declaring_class == _declaring_class)
		delete _slot;
}

llvm::Value* lcool::cool_method::call(
	llvm::IRBuilder<> builder,
	const std::vector<llvm::Value*>& args,
	bool static_call) const
{
	assert(_func != nullptr);
	assert(!_func->arg_empty());
	assert(args.size() >= 1);
	assert(_slot->vtable_index >= 0);

	// object must be the same type as _func's first argument
	llvm::Value* instance = *args.begin();
	assert(_func->getFunctionType()->getParamType(0) == instance->getType());

	// Upcast to object
	llvm::Value* instance_upcast = _slot->declaring_class->upcast_to_object(builder, instance);

	// Call null_check on the object
	llvm::Module* module = _func->getParent();
	llvm::Function* null_check_func = module->getFunction("null_check");
	assert(null_check_func != nullptr);

	builder.CreateCall(null_check_func, instance_upcast);

	// If declaring class is final, this can be a static call
	if (_declaring_class->is_final())
		static_call = true;

	// Get function to call
	llvm::Value* func;
	if (static_call)
	{
		func = _func;
	}
	else
	{
		// Get pointer to vtable
		llvm::Type* ptr_vtable_type = _slot->declaring_class->llvm_vtable()->getType()->getPointerTo();

		llvm::Value* zero = builder.getInt32(0);
		std::vector<llvm::Value*> gep_args1{ zero, zero };
		llvm::Value* ptr_ptr_vtable = builder.CreateInBoundsGEP(instance_upcast, gep_args1);
		llvm::Value* ptr_obj_vtable = builder.CreateLoad(ptr_ptr_vtable);
		llvm::Value* ptr_vtable = builder.CreateBitCast(ptr_obj_vtable, ptr_vtable_type);

		// Get destination function pointer
		llvm::Value* vtable_index = builder.getInt32(_slot->vtable_index);
		std::vector<llvm::Value*> gep_args2{ zero, vtable_index };
		llvm::Value* ptr_func = builder.CreateInBoundsGEP(ptr_vtable, gep_args2);
		func = builder.CreateLoad(ptr_func);
	}

	// Call it
	return builder.CreateCall(func, args);
}

// ========= cool_class ==========================================

lcool::cool_class::cool_class(const std::string& name, const cool_class* parent)
	: _name(name), _parent(parent)
{
}

bool lcool::cool_class::is_subclass_of(const cool_class* other) const
{
	return other == this ||
	       (_parent != nullptr &&  _parent->is_subclass_of(other));
}

bool lcool::cool_class::is_final() const
{
	return false;
}

const cool_attribute* lcool::cool_class::lookup_attribute(const std::string& name) const
{
	auto iter = _attributes.find(name);
	if (iter != _attributes.end())
		return iter->second.get();

	return nullptr;
}

const cool_method* lcool::cool_class::lookup_method(const std::string& name, bool recursive) const
{
	// Try this class first
	auto iter = _methods.find(name);
	if (iter != _methods.end())
		return iter->second.get();

	// Try subclasses
	if (recursive && _parent != nullptr)
		return _parent->lookup_method(name, true);

	return nullptr;
}

llvm::Value* lcool::cool_class::create_object(llvm::IRBuilder<> builder) const
{
	return builder.CreateCall(_constructor);
}

llvm::Value* lcool::cool_class::upcast_to(llvm::IRBuilder<> builder, llvm::Value* value, const cool_class* to) const
{
	// Handle trivial case
	if (to == this)
		return value;

	// We want to create a GEP with n zeros so that we get the correct struct type
	int num_zeros = 1;
	const cool_class* current = this;

	while (current != to)
	{
		assert(current != nullptr);
		current = current->_parent;
		num_zeros++;
	}

	// Do the upcast
	std::vector<llvm::Value*> gep_args(num_zeros, builder.getInt32(0));
	return builder.CreateInBoundsGEP(value, gep_args);
}

llvm::Value* lcool::cool_class::upcast_to_object(llvm::IRBuilder<> builder, llvm::Value* value) const
{
	const cool_class* cls = this;
	while (cls->_parent != nullptr)
		cls = cls->_parent;

	return upcast_to(builder, value, cls);
}

llvm::Value* lcool::cool_class::downcast(llvm::IRBuilder<> builder, llvm::Value* value) const
{
	return builder.CreateBitCast(value, _llvm_type->getPointerTo());
}

void lcool::cool_class::refcount_inc(llvm::IRBuilder<> builder, llvm::Value* value) const
{
	// Call refcount_inc on value given
	builder.CreateCall(_vtable->getParent()->getFunction("refcount_inc"), value);
}

void lcool::cool_class::refcount_dec(llvm::IRBuilder<> builder, llvm::Value* value) const
{
	// Call refcount_dec on value given
	builder.CreateCall(_vtable->getParent()->getFunction("refcount_dec"), value);
}

// ========= cool_user_class ====================================

lcool::cool_user_class::cool_user_class(const std::string& name, const cool_class* parent)
	: cool_class(name, parent)
{
}

lcool::cool_user_class::cool_user_class(std::string&& name, const cool_class* parent)
	: cool_class(std::move(name), parent)
{
}

bool lcool::cool_user_class::insert_attribute(unique_ptr<cool_attribute> attribute)
{
	std::string name = attribute->name;
	auto iter = _attributes.emplace(std::move(name), std::move(attribute));
	return iter.second;
}

bool lcool::cool_user_class::insert_method(unique_ptr<cool_method> method)
{
	std::string name = method->slot()->name;
	auto iter = _methods.emplace(std::move(name), std::move(method));
	return iter.second;
}

// ========= cool_program =======================================

lcool::cool_program::cool_program(llvm::LLVMContext& context)
	: _module("lcool_program", context)
{
}

cool_class* lcool::cool_program::lookup_class(const std::string& name)
{
	auto iter = _classes.find(name);
	if (iter != _classes.end())
		return iter->second.get();

	return nullptr;
}

const cool_class* lcool::cool_program::lookup_class(const std::string& name) const
{
	auto iter = _classes.find(name);
	if (iter != _classes.end())
		return iter->second.get();

	return nullptr;
}

cool_class* lcool::cool_program::insert_class(unique_ptr<cool_class> cls)
{
	std::string name = cls->name();
	auto result = _classes.emplace(name, std::move(cls));
	if (result.second)
		return result.first->second.get();

	return nullptr;

}
