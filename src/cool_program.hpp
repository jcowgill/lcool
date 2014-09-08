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
#include <map>
#include <string>

#include "smart_ptr.hpp"

namespace lcool
{
	/*
	Program
	=======
	List of all classes
	LLVM module where code for everything is stored
	- Global functions are accessed directly with module.getFunction

	Class
	=====
	Name
	Reference to parent class (or null for object)
	List of attributes
	List of methods
	Reference to vtable object
	Reference to structure type
	Constructor function
	Object size (bytes)
	 Calculated and stored in vtable (not used directly otherwise)

	Func: Create object
	 Allocate and call constructor
	 Noop for Int and Bool (returns 0/false)

	Func: Upcast to parent (fails if this is object)
	Func: Upcast to Object (noop if this is object)
	 For Int and Bool, these functions will box the value

	Func: Downcast from Object (or whatever)
	 For Int and Bool, this unboxes the given value
	 For others, this is just a bitcast

	Func: Inc refcount
	Func: Dec refcount
	 Noop for Int and Bool

	----
	Each class needs generating
	 Class structure type
	 Class vtable object
	 *Constructor function
	 *Copy constructor function
	 *Destructor
	 Slots for all methods
	 Slots for all attributes

	 * If applicable

	Attribute
	=========
	Name
	Type (reference to class object)
	Structure Index (nth item in class structure, NOT IN BYTES)

	Method
	======
	Name
	Return type
	List of types of arguments
	LLVM function (not for builtin / special methods)

	new
	override
	special (string ops)

	Func: Call method
	 */

	class cool_class;

	/** Contains information about an attribute */
	struct cool_attribute
	{
		/** The name of this attribute */
		std::string name;

		/** The type of this attribute */
		const cool_class& type;

		/**
		 * The index into the llvm_type of the parent class this attribute is stored at
		 *
		 * This variable is invalid until the parent class has been baked.
		 */
		int slot;
	};

	/** Contains information about a method */
	class cool_method
	{
	public:
		virtual ~cool_method() = default;

		/** The name of this method */
		const std::string& name() const;

		/** The return type of this method */
		const cool_class& return_type() const;

		/** A list containing the types of this method's parameters */
		const std::vector<const cool_class*>& parameter_types() const;

		/** Returns this method's LLVM function */
		const llvm::Function* llvm_func() const;

		/**
		 * Creates a call instruction to call this method
		 *
		 * @param builder the IR builder to insert the call into
		 * @param object the objet to call the method on (must be a class object)
		 * @param args list of arguments to pass to this method
		 * @return a value representing the return value of the call
		 */
		virtual llvm::Value* call(
			llvm::IRBuilder<> builder,
			llvm::Value* object,
			std::initializer_list<llvm::Value*> args) = 0;

		/** Bakes this method */
		virtual void bake() = 0;

		/** Returns true if this method has been baked */
		virtual bool is_baked() const = 0;

	protected:
		cool_method(const std::string& name, const cool_class& return_type);
		cool_method(std::string&& name, const cool_class& return_type);

		std::string _name;
		const cool_class& _return_type;
		std::vector<const cool_class*> _parameter_types;
		llvm::Function* _func = nullptr;
	};

	/**
	 * A method which is called using a vtable
	 *
	 * The majority of methods fall under this class.
	 * The main exception are the string method which are called directly.
	 */
	class cool_vtable_method : public cool_method
	{
	public:
		// See cool_method for docs
		virtual llvm::Value* call(
			llvm::IRBuilder<> builder,
			llvm::Value* object,
			std::initializer_list<llvm::Value*> args);

	protected:
		cool_vtable_method(const std::string& name, const cool_class& return_type);
		cool_vtable_method(std::string&& name, const cool_class& return_type);

		// The location of the function pointer for this method
		llvm::StructType* _vtable_type = nullptr;
		int _vtable_index = 0;
	};

	/** A user-defined method */
	class cool_user_method : public cool_vtable_method
	{
	public:
		/** Creates a new user-defined method */
		cool_user_method(const std::string& name, const cool_class& return_type);
		cool_user_method(std::string&& name, const cool_class& return_type);

		/**
		 * Adds a parameter to this method's list of parameters
		 *
		 * It is illegal to call this method after this method is baked
		 */
		void add_parameter(const cool_class& type);

		/** Returns this method's LLVM function */
		llvm::Function* llvm_func();

		// See cool_method for docs
		virtual void bake();
		virtual bool is_baked() const;
	};

	/** Contains the LLVM structure of a cool class */
	class cool_class
	{
	public:
		cool_class(const std::string& name, const cool_class* parent);
		cool_class(std::string&& name, const cool_class* parent);
		virtual ~cool_class() = default;

		/** Returns the name of this class */
		const std::string& name() const;

		/** Returns the parent of this class or NULL if this is the Object class */
		const cool_class* parent() const;

		/**
		 * Lookup an attribute by its name
		 * @return a pointer to the attribute or NULL if the attribute does not exist
		 */
		const cool_attribute* lookup_attribute(const std::string& name) const;

		/**
		 * Lookup a method by its name
		 *
		 * @param name name of the method
		 * @param recursive search parent classes in addition to this class
		 * @return a pointer to the method or NULL if the method does not exist
		 */
		cool_method* lookup_method(const std::string& name);
		const cool_method* lookup_method(const std::string& name, bool recursive = false) const;

		/**
		 * Inserts an attribute into the class
		 *
		 * It is illegal to call this method after this class is baked
		 */
		bool insert_attribute(unique_ptr<cool_attribute> attribute);

		/**
		 * Inserts a method into the class
		 *
		 * It is illegal to call this method after this class is baked
		 */
		bool insert_method(unique_ptr<cool_method> method);

		/**
		 * Returns the LLVM structure used for this class
		 *
		 * It is illegal to call this method before the class has been baked.
		 */
		const llvm::StructType& llvm_type() const;

		/**
		 * Returns the LLVM vtable object used for this class
		 *
		 * It is illegal to call this method before the class has been baked.
		 */
		const llvm::GlobalVariable& llvm_vtable() const;

		/**
		 * Bakes this class
		 *
		 * Baking the class finalizes the LLVM structures preparing for code generation.
		 * After this method has been called no structural modifications to this class are not allowed.
		 * Calling this method multiple times has no effect.
		 */
		virtual void bake() = 0;

		/** Returns true if this class has been baked */
		virtual bool is_baked() const = 0;

	protected:
		std::string _name;
		const cool_class* _parent = nullptr;
		std::map<std::string, unique_ptr<cool_attribute>> _attributes;
		std::map<std::string, unique_ptr<cool_method>> _methods;

		llvm::StructType* _llvm_type = nullptr;
		llvm::GlobalVariable* _vtable = nullptr;
	};

	/** A user-defined class */
	class cool_user_class : public cool_class
	{
	public:
		/** Creates a new user-defined class */
		cool_user_class(const std::string& name, const cool_class* parent);
		cool_user_class(std::string&& name, const cool_class* parent);

		// See cool_class for docs
		virtual void bake();
		virtual bool is_baked() const;
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
		llvm::Module& module();
		const llvm::Module& module() const;

		/**
		 * Lookup a class by its name
		 * @return a pointer to the class or NULL if the class does not exist
		 */
		cool_class* lookup_class(const std::string& name);
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
		 * Bakes this program and all the classes
		 *
		 * Baking the program finalizes the LLVM structures in preparation for code generation.
		 * After this method has been called no structural modifications to this program or any of
		 * the classes are allowed. Calling this method multiple times has no effect.
		 */
		void bake();

		/** Returns true if this program has been baked */
		bool is_baked() const;

	private:
		bool _baked;
		std::map<std::string, unique_ptr<cool_class>> _classes;
		llvm::Module _module;
	};

}

#endif
