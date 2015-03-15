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

#ifndef LCOOL_COOL_PROGRAM_HPP
#define LCOOL_COOL_PROGRAM_HPP

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <string>
#include <unordered_map>

#include "smart_ptr.hpp"

namespace lcool
{
	class cool_class;

	/** Contains information about an attribute */
	struct cool_attribute
	{
		/** The name of this attribute */
		std::string name;

		/** The type of this attribute */
		cool_class* type;

		/** The index into the llvm_type of the parent class this attribute is stored at */
		int slot;
	};

	/** Contains information about a method slot */
	struct cool_method_slot
	{
		/** The name of this method */
		std::string name;

		/** The return type of this method */
		cool_class* return_type;

		/** A list containing the types of this method's parameters */
		std::vector<cool_class*> parameter_types;

		/**
		 * The class which this method is declared in
		 *
		 * Since this is a slot class, this is always the class the method
		 * was originally declared in.
		 */
		cool_class* declaring_class;

		/** Index within the vtable a pointer to this method is found */
		int vtable_index;
	};

	/**
	 * Contains information about a method definition
	 *
	 * Each time a subclass overrides a method, a new instance of this class is
	 * created but the old instance of cool_method_slot is reused.
	 *
	 * The memory allocated for the method's slot is owned by the base method
	 * (ie the method where declaring_class == slot.declaring_class).
	 */
	class cool_method
	{
	public:
		/**
		 * Creates a new base method using a given method slot
		 *
		 * This method will take ownership of the slot. declaring_class will
		 * be set to slot.declaring_class.
		 *
		 * @param slot the method slot this method will own
		 * @param func the LLVM function of this specific method
		 */
		cool_method(std::unique_ptr<cool_method_slot> slot, llvm::Function* func);

		/**
		 * Creates a new override method
		 *
		 * The slot will be taken from the base class and declaring_class set
		 * accordingly.
		 *
		 * @param declaring_class the class which is declaring this specific method
		 * @param base_method the method overridden by this one
		 * @param func the LLVM function of this specific method
		 */
		cool_method(cool_class* declaring_class, cool_method* base_method, llvm::Function* func);

		/** Destructs this method (and possibly its slot) */
		virtual ~cool_method();

		/** This method's slot data */
		cool_method_slot* slot()
		{
			return _slot;
		}

		/** This method's slot data */
		const cool_method_slot* slot() const
		{
			return _slot;
		}

		/** Returns this method's LLVM function */
		llvm::Function* llvm_func()
		{
			return _func;
		}

		/** Returns this method's LLVM function */
		const llvm::Function* llvm_func() const
		{
			return _func;
		}

		/**
		 * The class which this method is defined in
		 *
		 * This is the class this particular override was defined in. It might
		 * be different to slot.declaring_class.
		 */
		cool_class* declaring_class()
		{
			return _declaring_class;
		}

		/**
		 * Creates a call instruction to call this method
		 *
		 * The first argument must be an instance of the declaring class of this
		 * method's slot (ie you probably need to upcast it first). A runtime
		 * error is generated if this argument is null.
		 *
		 * @param builder the IR builder to insert the call into
		 * @param args list of arguments to pass to this method
		 * @param static_call call the method statically
		 * @return a value representing the return value of the call
		 */
		virtual llvm::Value* call(
			llvm::IRBuilder<> builder,
			const std::vector<llvm::Value*>& args,
			bool static_call = false) const;

	private:
		cool_method() = default;
		cool_method(const cool_method&) = delete;
		cool_method& operator=(const cool_method&) = delete;

		cool_method_slot* _slot;
		cool_class* _declaring_class;
		llvm::Function* _func;
	};

	/** Contains the LLVM structure of a cool class */
	class cool_class
	{
	public:
		cool_class(const std::string& name, cool_class* parent);
		virtual ~cool_class() = default;

		/** Returns the name of this class */
		const std::string& name() const
		{
			return _name;
		}

		/** Returns the parent of this class or NULL if this is the Object class */
		cool_class* parent()
		{
			return _parent;
		}

		/** Returns true if this class is a subclass of some other class */
		bool is_subclass_of(const cool_class* other) const;

		/** Returns true if this class is final (can't be inherited from) */
		virtual bool is_final() const;

		/**
		 * Lookup an attribute by its name
		 * @return a pointer to the attribute or NULL if the attribute does not exist
		 */
		cool_attribute* lookup_attribute(const std::string& name);

		/**
		 * Lookup a method by its name
		 *
		 * @param name name of the method
		 * @param recursive search parent classes in addition to this class
		 * @return a pointer to the method or NULL if the method does not exist
		 */
		cool_method* lookup_method(const std::string& name, bool recursive = false);

		/** Returns a vector containing all the attributes declared in this class */
		std::vector<cool_attribute*> attributes();

		/** Returns a vector containing all the methods declared in this class */
		std::vector<cool_method*> methods();

		/**
		 * Returns the LLVM type used for this class
		 *
		 * This is usually a PointerType, except for Int and Bool where it is
		 * an IntegerType. The boxed Int and Bool struct types are not
		 * accessible (they should be treated as "opaque").
		 */
		llvm::Type* llvm_type()
		{
			return _llvm_type;
		}

		/**
		 * Returns the LLVM vtable object used for this class
		 *
		 * This is null for Int and Bool - if you want to call any methods on
		 * them, you need to upcast first.
		 */
		llvm::GlobalVariable* llvm_vtable()
		{
			return _vtable;
		}

		/**
		 * Creates an instance of this object
		 *
		 * All attributes will be initialized to their default values.
		 * For Int and Bool, this returns (i32 0) or (i1 0) respectively.
		 * For String, returns the empty string.
		 */
		virtual llvm::Value* create_object(llvm::IRBuilder<> builder) const;

		/**
		 * Upcasts an object of this class's type to one of it's parent types
		 *
		 * For Int and Bool, upcasting to Object will box the value.
		 */
		virtual llvm::Value* upcast_to(llvm::IRBuilder<> builder, llvm::Value* value, const cool_class* to) const;

		/**
		 * Helper method to upcast to Object
		 */
		llvm::Value* upcast_to_object(llvm::IRBuilder<> builder, llvm::Value* value) const;

		/**
		 * Statically downcasts a value to this class's type.
		 *
		 * This is a bitcast, so you must be sure the value is of this class's type!
		 * For Int and Bool, this will unbox the value.
		 */
		virtual llvm::Value* downcast(llvm::IRBuilder<> builder, llvm::Value* value) const;

		/**
		 * Increment the refcount on an object
		 */
		virtual void refcount_inc(llvm::IRBuilder<> builder, llvm::Value* value) const;

		/**
		 * Decrement the refcount on an object (and possibly free it)
		 */
		virtual void refcount_dec(llvm::IRBuilder<> builder, llvm::Value* value) const;

	protected:
		std::string _name;
		cool_class* _parent = nullptr;
		std::unordered_map<std::string, unique_ptr<cool_attribute>> _attributes;
		std::unordered_map<std::string, unique_ptr<cool_method>> _methods;

		llvm::Type* _llvm_type = nullptr;
		llvm::GlobalVariable* _vtable = nullptr;
	};

	/**
	 * Contains the LLVM structure of a cool program
	 *
	 * This class controls the lifetime of the entire class heirarchy.
	 * When this class is destroyed, all cool_class objects will also be destroyed.
	 */
	class cool_program
	{
	public:
		/** Creates an empty cool program with a LLVM module using the given context */
		explicit cool_program(llvm::LLVMContext& context);

		/** Returns this program's LLVM module */
		llvm::Module* module()
		{
			return &_module;
		}

		const llvm::Module* module() const
		{
			return &_module;
		}

		/**
		 * Lookup a class by its name
		 * @return a pointer to the class or NULL if the class does not exist
		 */
		cool_class* lookup_class(const std::string& name);

		/**
		 * Lookup a class by its name
		 * @return a pointer to the class or NULL if the class does not exist
		 */
		const cool_class* lookup_class(const std::string& name) const;

		/**
		 * Inserts a class into the program
		 *
		 * Note that the class supplied will be destroyed if this method fails.
		 *
		 * @return a pointer to the inserted class or NULL if a class with that name already exists
		 */
		cool_class* insert_class(unique_ptr<cool_class> cls);

		/**
		 * Constructs and inserts a class into the program
		 *
		 * @return a pointer to the inserted class or NULL if a class with that name already exists
		 * @see insert_class(unique_ptr<cool_class>)
		 */
		template <typename T, typename... Params>
		T* insert_class(Params... params)
		{
			return static_cast<T*>(insert_class(make_unique<T>(params...)));
		}

		/**
		 * Creates a string literal constant
		 *
		 * Remember to increment the refcount if the returned value is stored anywhere.
		 *
		 * @param content the content of the string
		 * @param name the global name to give to this constant (may be "")
		 * @return a string value (type %String*)
		 */
		llvm::Constant* create_string_literal(std::string content, std::string name = "");

	private:
		std::unordered_map<std::string, unique_ptr<cool_class>> _classes;
		llvm::Module _module;
	};
}

#endif
