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
#include "program_structure.hpp"

namespace ast = lcool::ast;

using lcool::attribute_structure;
using lcool::class_structure;
using lcool::method_structure;
using lcool::program_structure;
using lcool::unique_ptr;

lcool::program_structure::program_structure(const ast::program& program, llvm::LLVMContext& context)
	: _program(program), _module("lcool_prog", context)
{
}

void lcool::program_structure::generate_structure(lcool::logger& log)
{
	std::set<class_structure*> analyzed, open;

	// Do sanity check
	assert(_class_index.empty());

	// Insert builtins
	add_builtins(*this);

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

const ast::program& lcool::program_structure::program() const
{
	return _program;
}

class_structure* lcool::program_structure::insert_class(const std::string& name, class_structure&& cls)
{
	auto result = _class_index.emplace(name, std::move(cls));
	if (result.second)
		return &result.first->second;

	return nullptr;
}

const class_structure* lcool::program_structure::lookup_class(const std::string& name) const
{
	auto iter = _class_index.find(name);
	if (iter != _class_index.end())
		return &iter->second;

	return nullptr;
}

llvm::Module& lcool::program_structure::llvm_module()
{
	return _module;
}

const llvm::Module& lcool::program_structure::llvm_module() const
{
	return _module;
}
