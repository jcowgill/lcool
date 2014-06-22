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

#ifndef LCOOL_PROGRAM_STRUCTURE_HPP
#define LCOOL_PROGRAM_STRUCTURE_HPP

#include <llvm/IR/Constants.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <map>
#include <string>

#include "ast.hpp"
#include "logger.hpp"
#include "smart_ptr.hpp"

namespace lcool
{
	class program_structure;

	/** Contains the LLVM structure of an attribute */
	class attribute_structure
	{
	public:
		/** Returns the AST node for this attribute */
		const ast::attribute& attribute() const;

	private:
		// Base data
		const ast::attribute& _attribute;

		// Analyzed data
		unsigned _class_index;
	};

	/** Contains the LLVM structure of a method */
	class method_structure
	{
	public:
		/**
		 * Returns the AST node for this method
		 *
		 * Calling this on methods of builtin classes (which have no AST node) is not allowed
		 */
		const ast::method& method() const;

	private:
		// Base data
		const ast::method* _method;

		// Analyzed data
		llvm::Function* _func;
		unsigned _vtable_index;
	};

	/** Type of classes requiring special handling */
	enum class special_class_type
	{
		normal,
		integer,
		boolean,
		string,
	};

	/** Contains the LLVM structure of a class */
	class class_structure
	{
	public:
		/** Initialize a class_structure using a class AST node */
		explicit class_structure(const ast::cls& cls);

		/** Initialize a class_structure for a builtin type (pre-analyzed) */
		class_structure(
			const class_structure* parent,
			special_class_type special_type,
			llvm::StructType* class_type,
			unique_ptr<llvm::GlobalVariable>&& vtable_value);

		// Non copyable, but move constructable
		class_structure(const class_structure&) = delete;
		class_structure(class_structure&&) = default;
		class_structure& operator =(const class_structure&) = delete;

		/** Resolves the parent type of this class */
		bool resolve_parent_type(const program_structure& program, logger& log);

		/**
		 * Performs the main analysis on this class
		 *
		 * - Indexes all features
		 * - Resolves all the types associated with features
		 * - Resolves method overrides
		 * - Generates the final LLVM structures (no code generated yet though)
		 */
		bool analyze(const program_structure& program, logger& log);

		/**
		 * Inserts a method into the method index
		 *
		 * @param name name of the method
		 * @param method method structure to move into the method index
		 * @return a pointer to the method or NULL if the method already exists
		 */
		method_structure* insert_method(const std::string& name, method_structure&& method);

		/**
		 * Inserts an attribute into the attribute index
		 *
		 * @param name name of the attribute
		 * @param attribute attribute structure to move into the attribute index
		 * @return a pointer to the attribute or NULL if the attribute already exists
		 */
		attribute_structure* insert_attribute(const std::string& name, attribute_structure&& attribute);

		/**
		 * Returns the AST node for this class
		 *
		 * Calling this on builtin classes (which have no AST node) is not allowed
		 */
		const ast::cls& cls() const;

		/**
		 * Returns the parent of this class or NULL if this is "Object"
		 */
		class_structure* parent() const;

		/** Returns true if this is a builtin class (and therefore has no AST node) */
		bool is_builtin() const;

		/** Returns the special type handling this class requires */
		special_class_type special_type() const;

		/** Returns true if this class is a subclass (or the same class) as other */
		bool is_subclass_of(const class_structure& other) const;

		/** Returns true if this class is a superclass (of the same class) as other */
		bool is_superclass_of(const class_structure& other) const;

		/**
		 * Lookup a method
		 *
		 * @param name the name of the method to lookup
		 * @param search_parents true to search parent classes if the method can't be found here
		 * @return the method or NULL if none was found
		 */
		const method_structure* lookup_method(const std::string& name, bool search_parents = true);

		/**
		 * Lookup an attribute
		 *
		 * @param name the name of the attribute to lookup
		 * @return the attribute or NULL if none was found
		 */
		const attribute_structure* lookup_attribute(const std::string& name);

		/**
		 * Returns the LLVM structure type used for this class
		 *
		 * For the "Object" class:
		 *  Offset | Content
		 *  ------------------------
		 *  0      | pointer to vtable
		 *  8      | reference count
		 *
		 * For other classes:
		 *  Offset | Content
		 *  ------------------------
		 *  0      | entire parent class
		 *  16+    | attributes of this class
		 *
		 * The attribute_structure class contains the index into this structure of each attribute.
		 */
		const llvm::StructType& llvm_type() const;

		/**
		 * Returns the LLVM structure type used for this class's vtable instance
		 *
		 * For the "Object" class:
		 *  Offset | Content
		 *  ------------------------
		 *  0      | pointer to parent vtable (or NULL for object)
		 *  8      | pointer to the type_name of this class (eg the string "Object" for Object)
		 *  16     | destructor ~SELF_TYPE()
		 *  24     | abort() : Object
		 *  32     | copy() : SELF_TYPE
		 *  40     | type_name() : String
		 *
		 * For other classes:
		 *  Offset | Content
		 *  ------------------------
		 *  0      | entire parent vtable
		 *  48+    | new (not overridden) methods of this class
		 *
		 * The method_structure class contains the index into this structure of each attribute.
		 */
		const llvm::StructType& llvm_vtable_type() const;

		/** Returns the vtable pointer for this class */
		const llvm::GlobalVariable& llvm_vtable_value() const;

	private:
		// Base data
		const ast::cls* _class = nullptr;
		std::map<std::string, method_structure> _method_index;
		std::map<std::string, attribute_structure> _attribute_index;

		// Analyzed data
		special_class_type _special_type = special_class_type::normal;
		const class_structure* parent_class = nullptr;

		llvm::StructType* _class_type = nullptr;
		unique_ptr<llvm::GlobalVariable> _vtable_value;
	};

	/** Contains the LLVM structure of a program */
	class program_structure
	{
	public:
		/** Constructs a program structure with the given program AST without doing any analysis */
		program_structure(const ast::program& program, llvm::LLVMContext& context);

		/**
		 * Generates the entire program structure
		 *
		 * Specifically this method:
		 * - Registers all the builtin classes with the program
		 * - Indexes all the classes, methods and attributes
		 * - Generates the LLVM types and function prototypes for all classes and methods
		 */
		void generate_structure(logger& log);

		/** Returns the AST node for this program */
		const ast::program& program() const;

		/**
		 * Inserts a class into the class index
		 *
		 * @param name name of the class
		 * @param cls class structure to move into the class index
		 * @return a pointer to the class or NULL if the class already exists
		 */
		class_structure* insert_class(const std::string& name, class_structure&& cls);

		/**
		 * Lookup a class
		 *
		 * @param name the name of the class to lookup
		 * @return the class or NULL if none was found
		 */
		const class_structure* lookup_class(const std::string& name) const;

		/** Returns the LLVM module used for this program */
		llvm::Module& llvm_module();
		const llvm::Module& llvm_module() const;

	private:
		// Base data
		const ast::program& _program;
		std::map<std::string, class_structure> _class_index;

		// Analyzed data
		llvm::Module _module;
	};
}

#endif
