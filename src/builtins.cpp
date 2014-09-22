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

#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/Linker/Linker.h>
#include <llvm/Support/MemoryBuffer.h>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>

#include "builtins.hpp"
#include "cool_program.hpp"

using namespace lcool;

using llvm::ErrorOr;
using llvm::Linker;
using llvm::MemoryBuffer;
using llvm::Module;
using llvm::StringRef;

// LLVM builtins bitcode file
static const unsigned char bitcode_data[] = {
#include "lcool_runtime.inc"
};

// Builtin pre-baked cool objects
namespace
{
	class cool_builtin_method : public cool_vtable_method
	{
	public:
		/** Construct a method with all the info needed */
		cool_builtin_method(
			const std::string& name,
			const cool_class& return_type,
			llvm::Function* func,
			llvm::StructType* vtable_type,
			int vtable_index)
			: cool_vtable_method(name, return_type)
		{
			_func = func;
			_vtable_type = vtable_type;
			_vtable_index = vtable_index;
		}

		cool_builtin_method& add_param(const cool_class* cls)
		{
			_parameter_types.push_back(cls);
			return *this;
		}

		void bake() override { }
		bool is_baked() const override { return true; }
	};

	class cool_builtin_static_method : public cool_method
	{
	public:
		/** Construct a method with all the info needed */
		cool_builtin_static_method(
			const std::string& name,
			const cool_class& return_type,
			llvm::Function* func)
			: cool_method(name, return_type)
		{
			_func = func;
		}

		cool_builtin_static_method& add_param(const cool_class* cls)
		{
			_parameter_types.push_back(cls);
			return *this;
		}

		virtual llvm::Value* call(
			llvm::IRBuilder<> builder,
			llvm::Value* object,
			std::initializer_list<llvm::Value*> args) override
		{
			#warning TODO implement static method call function
			return nullptr;
		}

		void bake() override { }
		bool is_baked() const override { return true; }
	};

	class cool_builtin_class : public cool_class
	{
	public:
		/** Creates a builtin class and extracts the LLVM structures from the given module */
		cool_builtin_class(Module* module, const std::string& name, const cool_class* parent)
			: cool_class(name, parent), _module(module)
		{
			// Get LLVM objects
			_llvm_type = module->getTypeByName(name);
			_vtable = module->getNamedGlobal(name + "$vtable");

			assert(_llvm_type != nullptr);
			assert(_vtable != nullptr);
		}

		/** Creates a new vtable method using a new vtable slot */
		cool_builtin_method& add_method(
			const std::string& name,
			const cool_class* return_type,
			int vtable_index)
		{
			// Get LLVM objects
			llvm::Function* func = _module->getFunction(name);
			llvm::StructType* vtable_type = llvm::cast<llvm::StructType>(_vtable->getType()->getElementType());

			assert(func != nullptr);

			// Create and insert method object
			auto result = _methods.emplace(name, make_unique<cool_builtin_method>(name, *return_type, func, vtable_type, vtable_index));
			assert(result.second);

			return static_cast<cool_builtin_method&>(*result.first->second);
		}

		/** Creates a new static method */
		cool_builtin_static_method& add_static_method(
			const std::string& name,
			const cool_class* return_type)
		{
			// Get LLVM objects
			llvm::Function* func = _module->getFunction(name);
			assert(func != nullptr);

			// Create and insert method object
			auto result = _methods.emplace(name, make_unique<cool_builtin_static_method>(name, *return_type, func));
			assert(result.second);

			return static_cast<cool_builtin_static_method&>(*result.first->second);
		}

		void bake() override { }
		bool is_baked() const override { return true; }

	private:
		Module* _module;
	};
}

// Link the runtime file into the given module
static void link_runtime(Module* dest)
{
	// Parse bitcode_data to get an LLVM module
	StringRef bitcode_data_ref(reinterpret_cast<const char*>(bitcode_data), sizeof(bitcode_data));
	unique_ptr<MemoryBuffer> buf(MemoryBuffer::getMemBuffer(bitcode_data_ref, "", false));
	ErrorOr<Module*> src = llvm::parseBitcodeFile(buf.get(), dest->getContext());

	// Link module into main program
	if (!src || !Linker::LinkModules(dest, *src, Linker::DestroySource, nullptr))
	{
		// Failed to link runtime (should never happen)
		std::cerr << "fatal error: failed to load lcool runtime bitcode file" << std::endl;
		std::abort();
	}
}

// Wrapper around program.insert_class to make calls to it more consise
static cool_builtin_class* insert_builtin_class(lcool::cool_program& program, const std::string& name, const cool_class* parent)
{
	return static_cast<cool_builtin_class*>(program.insert_class(make_unique<cool_builtin_class>(&program.module(), name, parent)));
}

void lcool::load_builtins(lcool::cool_program& program)
{
	Module* module = &program.module();

	// Link in runtime library
	link_runtime(module);

	// Register builtin classes
	cool_builtin_class* cls_object = insert_builtin_class(program, "Object", nullptr);
	cool_builtin_class* cls_io     = insert_builtin_class(program, "IO",     cls_object);
	cool_builtin_class* cls_string = insert_builtin_class(program, "String", cls_object);
	cool_builtin_class* cls_int    = insert_builtin_class(program, "Int",    cls_object);
	cool_builtin_class* cls_bool   = insert_builtin_class(program, "Bool",   cls_object);

	assert(cls_object && cls_io && cls_string  && cls_int && cls_bool);

#warning TODO Handle SELF_TYPE
	// Register methods
	cls_object->add_method("abort",     cls_object, 4);
	cls_object->add_method("copy",      cls_object, 5);
	cls_object->add_method("type_name", cls_string, 6);

	cls_io->add_method("in_int",     cls_int, 1);
	cls_io->add_method("in_string",  cls_string, 2);
	cls_io->add_method("out_int",    cls_io, 3).add_param(cls_int);
	cls_io->add_method("out_string", cls_io, 4).add_param(cls_string);

	cls_string->add_static_method("length", cls_int);
	cls_string->add_static_method("concat", cls_string).add_param(cls_string);
	cls_string->add_static_method("substr", cls_string).add_param(cls_int).add_param(cls_int);
}
