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

#include "builtins.hpp"
#include "cool_program.hpp"

using lcool::unique_ptr;

using lcool::cool_attribute;
using lcool::cool_method;
using lcool::cool_class;
using lcool::cool_program;

#if 0

lcool::cool_program::cool_program(const ast::program& program, llvm::LLVMContext& context)
	: _program(program), _module("lcool_prog", context)
{
}

void lcool::cool_program::generate_structure(lcool::logger& log)
{
	std::set<class_structure*> analyzed, open;

	// Do sanity check
	assert(_class_index.empty());

	// Insert builtins
	load_builtins(*this);

	// Add all builtins to analyzed set
	for (auto& pair : _class_index)
		analyzed.insert(&pair.second);

	// Index all classes first
	for (const ast::cls& cls : _program)
	{
		class_structure* structure = insert_class(cls.name, class_structure(cls));

		if (structure != nullptr)
		{
			open.insert(structure);
		}
		else
		{
#warning  Print error
		}
	}

	// Resolve parent classes
	for (auto& pair : _class_index)
	{
		if (!pair.second.resolve_parent_type(*this, log))
		{
			// Error resolving class, remove from open set
			assert(log.has_errors());
			open.erase(&pair.second);
		}
	}

	// We must analyze classes so that their parent types are analyzed first
	bool processed_something;
	do
	{
		processed_something = false;

		// Analyze any class we find with its parent in the analyzed set
		auto it = open.begin();
		while (it != open.end())
		{
			auto current = it++;
			auto cls_ptr = *current;

			if (analyzed.count(cls_ptr->parent()) == 1)
			{
				open.erase(current);
				if (cls_ptr->analyze(*this, log))
				{
					analyzed.insert(cls_ptr);
					processed_something = true;
				}
			}
		}
	}
	while (processed_something);

	// If the open set isn't empty, there must be an inheritance cycle
	if (!open.empty())
	{
#warning Print some error here
	}
}

const ast::program& lcool::cool_program::program() const
{
	return _program;
}

class_structure* lcool::cool_program::insert_class(const std::string& name, class_structure&& cls)
{
	auto result = _class_index.emplace(name, std::move(cls));
	if (result.second)
		return &result.first->second;

	return nullptr;
}

const class_structure* lcool::cool_program::lookup_class(const std::string& name) const
{
	auto iter = _class_index.find(name);
	if (iter != _class_index.end())
		return &iter->second;

	return nullptr;
}

llvm::Module& lcool::cool_program::llvm_module()
{
	return _module;
}

const llvm::Module& lcool::cool_program::llvm_module() const
{
	return _module;
}

llvm::GlobalValue* get_global(global_id id)
{
	return _globals[static_cast<int>(id)];
}

void set_global(global_id id, llvm::GlobalValue* value)
{
	int int_id = static_cast<int>(id);

	if (_globals.size() <= int_id)
		_globals.resize(int_id + 1);

	_globals[int_id] = value;
}

#endif

// ========= cool_method =======================================

lcool::cool_method::cool_method(const std::string& name, const cool_class& return_type)
	: _name(name), _return_type(return_type)
{
}

lcool::cool_method::cool_method(std::string&& name, const cool_class& return_type)
	: _name(std::move(name)), _return_type(return_type)
{
}

const std::string& lcool::cool_method::name() const
{
	return _name;
}

const cool_class& lcool::cool_method::return_type() const
{
	return _return_type;
}

const std::vector<const cool_class*>& lcool::cool_method::parameter_types() const
{
	return _parameter_types;
}

const llvm::Function* lcool::cool_method::llvm_func() const
{
	assert(is_baked());
	return _func;
}

// ========= cool_vtable_method ================================

lcool::cool_vtable_method::cool_vtable_method(const std::string& name, const cool_class& return_type)
	: cool_method(name, return_type)
{
}

lcool::cool_vtable_method::cool_vtable_method(std::string&& name, const cool_class& return_type)
	: cool_method(std::move(name), return_type)
{
}

llvm::Value* lcool::cool_vtable_method::call(
	llvm::IRBuilder<> builder,
	llvm::Value* object,
	std::initializer_list<llvm::Value*> args)
{
	#warning TODO lcool::cool_vtable_method::call
	return nullptr;
}

// ========= cool_user_method ==================================

lcool::cool_user_method::cool_user_method(const std::string& name, const cool_class& return_type)
	: cool_vtable_method(name, return_type)
{
}

lcool::cool_user_method::cool_user_method(std::string&& name, const cool_class& return_type)
	: cool_vtable_method(std::move(name), return_type)
{
}

void lcool::cool_user_method::add_parameter(const cool_class& type)
{
	assert(!is_baked());
	_parameter_types.push_back(&type);
}

llvm::Function* lcool::cool_user_method::llvm_func()
{
	assert(is_baked());
	return _func;
}

void lcool::cool_user_method::bake()
{
	#warning TODO lcool::cool_user_method::bake
}

bool lcool::cool_user_method::is_baked() const
{
	return _vtable_type != nullptr;
}

// ========= cool_class =========================================

const std::string& lcool::cool_class::name() const
{
	return _name;
}

const cool_class* lcool::cool_class::parent() const
{
	return _parent;
}

const cool_attribute* lcool::cool_class::lookup_attribute(const std::string& name) const
{
	auto iter = _attributes.find(name);
	if (iter != _attributes.end())
		return iter->second.get();

	return nullptr;
}

cool_method* lcool::cool_class::lookup_method(const std::string& name)
{
	auto iter = _methods.find(name);
	if (iter != _methods.end())
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

// ========= cool_user_class ====================================

lcool::cool_user_class::cool_user_class(const std::string& name, const cool_class* parent)
{
	_name = name;
	_parent = parent;
}

lcool::cool_user_class::cool_user_class(std::string&& name, const cool_class* parent)
{
	_name = std::move(name);
	_parent = parent;
}

bool lcool::cool_user_class::insert_attribute(unique_ptr<cool_attribute> attribute)
{
	assert(!is_baked());

	std::string name = attribute->name;
	auto iter = _attributes.emplace(std::move(name), std::move(attribute));
	return iter.second;
}

bool lcool::cool_user_class::insert_method(unique_ptr<cool_method> method)
{
	assert(!is_baked());

	std::string name = method->name();
	auto iter = _methods.emplace(std::move(name), std::move(method));
	return iter.second;
}

const llvm::StructType& lcool::cool_user_class::llvm_type() const
{
	assert(is_baked());
	return *_llvm_type;
}

const llvm::GlobalVariable& lcool::cool_user_class::llvm_vtable() const
{
	assert(is_baked());
	return *_vtable;
}

void lcool::cool_user_class::bake()
{
	#warning TODO lcool::cool_user_class::bake
}

bool lcool::cool_user_class::is_baked() const
{
	return _vtable != nullptr;
}


// ========= cool_program =======================================

lcool::cool_program::cool_program(llvm::LLVMContext& context)
	: _module("lcool program", context)
{
}

llvm::Module& lcool::cool_program::module()
{
	return _module;
}

const llvm::Module& lcool::cool_program::module() const
{
	return _module;
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
	assert(!_baked);

	// Insert into classes list
	std::string name = cls->name();
	auto iter = _classes.emplace(std::move(name), std::move(cls));

	if (iter.second)
		return iter.first.second.get();

	return nullptr;
}

void lcool::cool_program::bake()
{
	// This program class has nothing special to be baked, so just bake all the classes
	if (!_baked)
	{
		for (auto& kv : _classes)
			kv.second->bake();

		_baked = true;
	}
}

bool lcool::cool_program::is_baked() const
{
	return _baked;
}
