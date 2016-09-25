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

#include <cassert>
#include <set>

#include "ast.hpp"
#include "builtins.hpp"
#include "cool_program.hpp"

using namespace lcool;

// ========= cool_method =========================================

lcool::cool_method::cool_method(std::unique_ptr<cool_method_slot> slot, llvm::Function* func)
	: _func(func)
{
	_slot = slot.release();
	_declaring_class = _slot->declaring_class;
}

lcool::cool_method::cool_method(cool_class* declaring_class, cool_method* base_method, llvm::Function* func)
	: _slot(base_method->slot()), _declaring_class(declaring_class), _func(func)
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

	// object must be the same type as _func's first argument
	assert(_func->getFunctionType()->getParamType(0) == args[0]->getType());

	// Upcast to object
	llvm::Value* self_upcast = _slot->declaring_class->upcast_to_object(builder, args[0]);

	// Call null_check on the object
	cool_program::call_global(_func->getParent(), builder, "null_check", self_upcast);

	// Always call statically if there is no vtable entry, or if the
	//  declaring class is final
	if (_slot->vtable_index == 0 || _declaring_class->is_final())
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
		llvm::Type* ptr_vtable_type = _slot->declaring_class->llvm_vtable()->getType();

		llvm::Value* zero = builder.getInt32(0);
		std::vector<llvm::Value*> gep_args1{ zero, zero };
		llvm::Value* ptr_ptr_vtable = builder.CreateInBoundsGEP(self_upcast, gep_args1);
		llvm::Value* ptr_obj_vtable = builder.CreateLoad(ptr_ptr_vtable);
		llvm::Value* ptr_vtable = builder.CreateBitCast(ptr_obj_vtable, ptr_vtable_type);

		// Get destination function pointer
		llvm::Value* vtable_index = builder.getInt32(_slot->vtable_index);
		std::vector<llvm::Value*> gep_args2{ zero, vtable_index };
		llvm::Value* ptr_func = builder.CreateInBoundsGEP(ptr_vtable, gep_args2);
		func = builder.CreateLoad(ptr_func);
	}

	// Call it
	auto call_inst = builder.CreateCall(func, args);
	call_inst->setCallingConv(llvm::CallingConv::Fast);
	return call_inst;
}

// ========= cool_class ==========================================

lcool::cool_class::cool_class(const std::string& name, cool_class* parent)
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

cool_attribute* lcool::cool_class::lookup_attribute(const std::string& name)
{
	auto iter = _attributes.find(name);
	if (iter != _attributes.end())
		return iter->second.get();

	return nullptr;
}

cool_method* lcool::cool_class::lookup_method(const std::string& name, bool recursive)
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

std::vector<cool_attribute*> lcool::cool_class::attributes()
{
	std::vector<cool_attribute*> result;
	for (auto& pair : _attributes)
		result.push_back(pair.second.get());
	return result;
}

std::vector<cool_method*> lcool::cool_class::methods()
{
	std::vector<cool_method*> result;
	for (auto& pair : _methods)
		result.push_back(pair.second.get());
	return result;
}

llvm::Value* lcool::cool_class::create_object(llvm::IRBuilder<>& builder) const
{
	// Invoke new_object on vtable pointer
	auto value = call_global(builder, "new_object", upcast_to_object(builder, _vtable));
	return downcast(builder, value);
}

llvm::Value* lcool::cool_class::upcast_to(llvm::IRBuilder<>& builder, llvm::Value* value, const cool_class* to) const
{
	// Handle trivial case
	if (to == this)
		return value;

	// We want to create a GEP with n zeros so that we get the correct struct type
	int num_zeros = 1;
	const cool_class* current = this;

	while (current != to)
	{
		// If current is null, to is not a subclass of this class
		if (current == nullptr)
			return nullptr;

		current = current->_parent;
		num_zeros++;
	}

	// Do the upcast
	std::vector<llvm::Value*> gep_args(num_zeros, builder.getInt32(0));
	return builder.CreateInBoundsGEP(value, gep_args);
}

llvm::Value* lcool::cool_class::upcast_to_object(llvm::IRBuilder<>& builder, llvm::Value* value) const
{
	const cool_class* cls = this;
	while (cls->_parent != nullptr)
		cls = cls->_parent;

	return upcast_to(builder, value, cls);
}

llvm::Value* lcool::cool_class::downcast(llvm::IRBuilder<>& builder, llvm::Value* value) const
{
	return builder.CreateBitCast(value, _llvm_type);
}

void lcool::cool_class::refcount_inc(llvm::IRBuilder<>& builder, llvm::Value* value) const
{
	// Call refcount_inc on value given
	call_global(builder, "refcount_inc", upcast_to_object(builder, value));
}

void lcool::cool_class::refcount_dec(llvm::IRBuilder<>& builder, llvm::Value* value) const
{
	// Call refcount_dec on value given
	call_global(builder, "refcount_dec", upcast_to_object(builder, value));
}

llvm::Function* lcool::cool_class::constructor()
{
	return get_object_vtable_func(3);
}

llvm::Function* lcool::cool_class::copy_constructor()
{
	return get_object_vtable_func(4);
}

llvm::Function* lcool::cool_class::destructor()
{
	return get_object_vtable_func(5);
}

llvm::Function* lcool::cool_class::get_object_vtable_func(unsigned index)
{
	assert(_vtable != nullptr);
	assert(_vtable->hasInitializer());

	// Get this class's vtable and traverse upwards until we reach Object
	llvm::ConstantStruct* current =
		llvm::cast<llvm::ConstantStruct>(_vtable->getInitializer());

	for(;;)
	{
		auto next = current->getOperand(0);
		auto next_struct = llvm::dyn_cast<llvm::ConstantStruct>(next);

		if (next_struct == nullptr)
			break;

		current = next_struct;
	}

	// Get relevant function pointer
	return llvm::cast<llvm::Function>(current->getOperand(index));
}

llvm::CallInst* lcool::cool_class::call_global(llvm::IRBuilder<>& builder, std::string name, llvm::Value* arg) const
{
	return cool_program::call_global(_vtable->getParent(), builder, name, arg);
}

// ========= cool_program =======================================

lcool::cool_program::cool_program(llvm::LLVMContext& context)
{
	// Load builtins
	_module = lcool::builtins_load_bitfile(context);
	lcool::builtins_register(*this);
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

llvm::Constant* lcool::cool_program::create_string_literal(std::string content, std::string name)
{
	llvm::LLVMContext& context = module()->getContext();

	// If content is zero, return the empty string
	if (content.empty())
		return module()->getGlobalVariable("String$empty");

	// Create array from content
	auto content_array = llvm::ConstantDataArray::getString(context, content, false);

	// Create type for this literal
	std::vector<llvm::Type*> elements;
	auto i32_type = llvm::Type::getInt32Ty(context);

	auto object_type = module()->getTypeByName("Object");
	elements.push_back(object_type);
	elements.push_back(i32_type);
	elements.push_back(content_array->getType());

	auto literal_type = llvm::StructType::create(elements);

	// Create the literal itself
	std::vector<llvm::Constant*> object_elements;
	object_elements.push_back(module()->getGlobalVariable("String$vtable"));
	object_elements.push_back(llvm::ConstantInt::get(i32_type, 1));

	std::vector<llvm::Constant*> str_elements;
	str_elements.push_back(llvm::ConstantStruct::get(object_type, object_elements));
	str_elements.push_back(llvm::ConstantInt::get(i32_type, content.size()));
	str_elements.push_back(content_array);

	auto literal_var = new llvm::GlobalVariable(
		*module(),
		literal_type,
		false,
		llvm::GlobalVariable::PrivateLinkage,
		llvm::ConstantStruct::get(literal_type, str_elements),
		name);

	// Cast variable to %String*
	auto str_type = module()->getTypeByName("String")->getPointerTo();
	return llvm::ConstantExpr::getBitCast(literal_var, str_type);
}

llvm::CallInst* lcool::cool_program::call_global(llvm::IRBuilder<>& builder, std::string name, llvm::Value* arg)
{
	return call_global(module(), builder, name, arg);
}

llvm::CallInst* lcool::cool_program::call_global(
	llvm::Module* module, llvm::IRBuilder<>& builder, std::string name, llvm::Value* arg)
{
	auto to_call = module->getFunction(name);
	auto call_inst = builder.CreateCall(to_call, arg);
	call_inst->setCallingConv(to_call->getCallingConv());
	return call_inst;
}
