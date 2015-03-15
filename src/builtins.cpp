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

// LLVM builtins bitcode file
static const unsigned char bitcode_data[] = {
#include "lcool_runtime.inc"
};

// Builtin pre-baked cool objects
namespace
{
	/** A builtin reference type class (Object, IO, String) */
	class builtin_ref_class : public cool_class
	{
	public:
		/** Creates a builtin class and extracts the LLVM structures from the given module */
		builtin_ref_class(llvm::Module* module, const std::string& name, const cool_class* parent)
			: cool_class(name, parent), _module(module)
		{
			// Add LLVM objects
			_llvm_type = module->getTypeByName(name);
			_vtable = module->getNamedGlobal(name + "$vtable");

			assert(_llvm_type != nullptr);
			assert(_vtable != nullptr);
		}

		/**
		 * Creates a new vtable method using a new vtable slot
		 *
		 * vtable_index must be provided if the class is not final
		 */
		void add_method(
			const std::string& name,
			const cool_class* return_type,
			int vtable_index,
			std::initializer_list<const cool_class*> param_types = {})
		{
			// Create method slot
			auto slot = make_unique<cool_method_slot>();
			slot->name = name;
			slot->return_type = return_type;
			slot->parameter_types.assign(param_types);
			slot->declaring_class = this;
			slot->vtable_index = vtable_index;

			// Create and insert method
			auto result = _methods.emplace(name, make_unique<cool_method>(std::move(slot)));
			assert(result.second);
		}

		/** Creates a new static method */
		void add_static_method(
			const std::string& name,
			const cool_class* return_type,
			std::initializer_list<const cool_class*> param_types = {})
		{
			add_method(name, return_type, -1, param_types);
		}

	protected:
		llvm::Module* _module;
	};

	// String have some slight adjustments to optimize things a bit
	class builtin_string_class : public builtin_ref_class
	{
	public:
		builtin_string_class(llvm::Module* module, const std::string& name, cool_class* parent)
			: builtin_ref_class(module, name, parent)
		{
		}

		virtual bool is_final() const override
		{
			return true;
		}

		virtual llvm::Value* create_object(llvm::IRBuilder<> builder) const
		{
			// Return the global empty string (after incrementing its refcount)
			auto empty_str = _module->getGlobalVariable("String$empty");
			refcount_inc(builder, empty_str);
			return empty_str;
		}
	};

	/** Builtin value class (Int and Bool) */
	class builtin_value_class : public cool_class
	{
	public:
		/** Creates a builtin class and extracts the LLVM structures from the given module */
		builtin_value_class(
			llvm::Module* module,
			const std::string& name,
			const cool_class* parent,
			llvm::IntegerType* type)
			: cool_class(name, parent), _module(module)
		{
			// Add LLVM objects
			_llvm_type = type;
			_vtable = module->getNamedGlobal(name + "$vtable");

			assert(_llvm_type != nullptr);
			assert(_vtable != nullptr);
		}

		virtual bool is_final() const override
		{
			return true;
		}


		virtual llvm::Value* create_object(llvm::IRBuilder<>) const
		{
			// Return zero value for this type
			return llvm::ConstantInt::get(_llvm_type, 0);
		}

		virtual llvm::Value* upcast_to(llvm::IRBuilder<> builder, llvm::Value* value, const cool_class* to) const
		{
			// Handle self
			if (to == this)
				return value;

			// Class to cast to must be Object
			assert(to == _parent);

			// Box this value
			return builder.CreateCall(_module->getFunction(_name + "$box"), value);
		}

		virtual llvm::Value* downcast(llvm::IRBuilder<> builder, llvm::Value* value) const
		{
			// Value must be object's type
			assert(value->getType() == _parent->llvm_type());

			// Unbox this value
			return builder.CreateCall(_module->getFunction(_name + "$unbox"), value);
		}

		virtual void refcount_inc(llvm::IRBuilder<>, llvm::Value*) const
		{
		}

		virtual void refcount_dec(llvm::IRBuilder<>, llvm::Value*) const
		{
		}

	private:
		llvm::Module* _module;
	};
}

// Link the runtime file into the given module
static void link_runtime(llvm::Module* dest)
{
	// Parse bitcode_data to get an LLVM module
	llvm::StringRef bitcode_data_ref(reinterpret_cast<const char*>(bitcode_data), sizeof(bitcode_data));
	unique_ptr<llvm::MemoryBuffer> buf(llvm::MemoryBuffer::getMemBuffer(bitcode_data_ref, "", false));
	llvm::ErrorOr<llvm::Module*> src = llvm::parseBitcodeFile(buf.get(), dest->getContext());

	// Link module into main program
	if (!src || llvm::Linker::LinkModules(dest, *src, llvm::Linker::DestroySource, nullptr))
	{
		// Failed to link runtime (should never happen)
		std::cerr << "fatal error: failed to load lcool runtime bitcode file" << std::endl;
		std::abort();
	}
}

void lcool::load_builtins(lcool::cool_program& program)
{
	llvm::Module* module = program.module();

	// Link in runtime library
	link_runtime(module);

	// Register builtin classes
	llvm::IntegerType* int1 = llvm::IntegerType::get(module->getContext(), 1);
	llvm::IntegerType* int32 = llvm::IntegerType::get(module->getContext(), 32);

	auto cls_object = program.insert_class<builtin_ref_class>(module, "Object", nullptr);
	auto cls_io     = program.insert_class<builtin_ref_class>(module, "IO", cls_object);
	auto cls_string = program.insert_class<builtin_string_class>(module, "String", cls_object);
	auto cls_bool   = program.insert_class<builtin_value_class>(module, "Bool", cls_object, int1);
	auto cls_int    = program.insert_class<builtin_value_class>(module, "Int", cls_object, int32);

	assert(cls_object && cls_io && cls_string && cls_int && cls_bool);

#warning TODO Handle SELF_TYPE
	// Register methods
	cls_object->add_method("abort",     cls_object, 6);
	cls_object->add_method("copy",      cls_object, 7);
	cls_object->add_method("type_name", cls_string, 8);

	cls_io->add_method("in_int",     cls_int, 1);
	cls_io->add_method("in_string",  cls_string, 2);
	cls_io->add_method("out_int",    cls_io, 3, { cls_int });
	cls_io->add_method("out_string", cls_io, 4, { cls_string });

	cls_string->add_static_method("length", cls_int);
	cls_string->add_static_method("concat", cls_string, { cls_string });
	cls_string->add_static_method("substr", cls_string, { cls_int, cls_int });
}
