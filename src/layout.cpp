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

#include <boost/format.hpp>
#include <boost/logic/tribool.hpp>

#include "ast.hpp"
#include "builtins.hpp"
#include "cool_program.hpp"
#include "layout.hpp"
#include "logger.hpp"

using namespace lcool;

using boost::format;
using boost::logic::tribool;
using boost::logic::indeterminate;

namespace
{

/** A user-defined class */
class user_class : public lcool::cool_class
{
public:
	user_class(const std::string& name, cool_class* parent)
		: cool_class(name, parent)
	{
		// Also create an empty StructType for this class
		//  This is needed to handle references to types which have not been
		//  layed out yet
		auto& llvm_context = parent->llvm_type()->getContext();
		_llvm_type = llvm::StructType::create(llvm_context, name)->getPointerTo();
	}

	// We know this must be a PointerType
	llvm::PointerType* llvm_type()
	{
		return llvm::cast<llvm::PointerType>(_llvm_type);
	}

	// Returns the underlying StructType
	llvm::StructType* llvm_struct_type()
	{
		return llvm::cast<llvm::StructType>(llvm_type()->getElementType());
	}

	using cool_class::_attributes;
	using cool_class::_methods;
	using cool_class::_vtable;
};

// Processing state of class sorter
struct insert_empty_classes_state
{
	const ast::program& input;
	cool_program& output;
	logger& log;

	std::unordered_map<std::string, unsigned> input_index;
	std::vector<const ast::cls*> layout_list;
	std::vector<tribool> visited_list;

	insert_empty_classes_state(
		const ast::program& iinput,
		cool_program& ioutput,
		logger& ilog)
		: input(iinput), output(ioutput), log(ilog)
	{
	}
};

void log_class_loop(insert_empty_classes_state& state, unsigned class_index)
{
	std::string loop_str = state.input[class_index].name;
	unsigned current_class = class_index;

	// Construct the loop string by traversing each class's parent until
	//  we get back to the start
	do
	{
		// Go to parent class
		current_class = state.input_index[*state.input[current_class].parent];

		// Print it
		loop_str += " -> ";
		loop_str += state.input[current_class].name;
	}
	while(current_class != class_index);

	// Log the error and exit
	state.log.error(state.input[class_index].loc, "circular inheritance: " + loop_str);
}

// Visits a single class and adds it to the output list (ensuring all it's
//  parents are added before it)
void insert_empty_classes_visit(insert_empty_classes_state& state, unsigned class_index)
{
	const ast::cls& cls = state.input[class_index];

	// If we're in the middle of visiting the class, we've found a loop!
	if (indeterminate(state.visited_list[class_index]))
	{
		log_class_loop(state, class_index);
	}
	else if (!state.visited_list[class_index])
	{
		// Lookup parent class
		std::string parent_name = cls.parent.get_value_or("Object");
		cool_class* parent = state.output.lookup_class(parent_name);

		if (parent == nullptr)
		{
			// Try the index to search for forward declarations
			auto parent_iter = state.input_index.find(parent_name);
			if (parent_iter == state.input_index.end())
			{
				// Can't find it anywhere!
				state.log.error(cls.loc, format("class not defined '%s'") % parent_name);
			}
			else
			{
				// Class exists, but has not been processed yet
				state.visited_list[class_index] = indeterminate;
				insert_empty_classes_visit(state, parent_iter->second);

				// Try lookup again
				parent = state.output.lookup_class(parent_name);
			}
		}

		// This class is done
		state.visited_list[class_index] = true;

		// If the parent is still null, there was an error somewhere so die now
		if (parent == nullptr)
			return;

		// Create class object
		state.output.insert_class<user_class>(cls.name, parent);

		// Add to layout list
		state.layout_list.push_back(&cls);
	}
}

// Inserts empty versions of classes into the output program
//  Returns a list of classes ordered such that each class's parent never
//  succeeds it in the list.
std::vector<const ast::cls*> insert_empty_classes(
	const ast::program& input, cool_program& output, logger& log)
{
	insert_empty_classes_state state(input, output, log);
	state.visited_list.resize(input.size());

	// Create an index of every class in the input
	for (unsigned i = 0; i < input.size(); i++)
	{
		if (output.lookup_class(input[i].name) != nullptr ||
			!state.input_index.insert(std::make_pair(input[i].name, i)).second)
		{
			log.error(input[i].loc, format("redefinition of class '%s'") % input[i].name);
		}
	}

	if (!log.has_errors())
	{
		// Run the visit method on every class in the program
		for (unsigned i = 0; i < input.size(); i++)
			insert_empty_classes_visit(state, i);
	}

	return state.layout_list;
}

// Creates a "fast" function with certain attributes applied
llvm::Function create_fast_function(
	llvm::Module* module, llvm::FunctionType* type, std::string name)
{
	llvm::Function* func = llvm::Function::Create(
		type, llvm::Function::InternalLinkage, name, module);
	func->setCallingConv(llvm::Fast);
	func->addFnAttr(llvm::Attribute::NoUnwind);
	return func;
}

// Processes a class's attributes and creates its llvm structure
void process_attributes(const ast::cls& ast_cls, user_class* cls, cool_program& output, logger& log)
{
	std::vector<llvm::Type*> elements;
	std::vector<cool_attribute*> attrib_int, attrib_bool;

	// Ensure the parent class is not final
	if (cls->parent()->is_final())
	{
		log.error(ast_cls.loc,
			format("'%s' cannot inherit from special class '%s'")
			% cls->name() % cls->parent()->name());
	}
	else
	{
		// Add parent class to structure
		//  We want to add the content of the parent rather than a pointer to it
		auto parent_pointer = llvm::cast<llvm::PointerType>(cls->parent()->llvm_type());
		elements.push_back(parent_pointer->getElementType());
	}

	for (const ast::attribute& ast_attrib : ast_cls.attributes)
	{
		cool_class* type = output.lookup_class(ast_attrib.type);
		if (type == nullptr)
		{
			log.error(ast_attrib.loc, "unknown type '" + ast_attrib.type + "'");
			continue;
		}

		// Create and insert cool_attribute
		auto result = cls->_attributes.emplace(ast_attrib.name, make_unique<cool_attribute>());
		if (!result.second)
		{
			log.error(ast_attrib.loc, "attribute already defined '" + ast_attrib.name + "'");
			continue;
		}

		cool_attribute* attrib = result.first->second.get();
		attrib->name = ast_attrib.name;
		attrib->type = type;
		attrib->slot = elements.size();

		// If the type is an integer or boolean, it will be added to the elements
		//  list later at the end of the struct to reduce the amount of padding
		auto int_type = llvm::dyn_cast<llvm::IntegerType>(type->llvm_type());
		if (int_type != nullptr)
		{
			if (int_type->getBitWidth() == 1)
				attrib_bool.push_back(attrib);
			else
				attrib_int.push_back(attrib);
		}
		else
		{
			elements.push_back(type->llvm_type());
		}
	}

	// Add any ints or bools to the elements list
	for (auto attrib : attrib_int)
	{
		attrib->slot = elements.size();
		elements.push_back(attrib->type->llvm_type());
	}

	for (auto attrib : attrib_bool)
	{
		attrib->slot = elements.size();
		elements.push_back(attrib->type->llvm_type());
	}

	// Finally, set the body of our llvm type
	cls->llvm_struct_type()->setBody(elements);
}

// Processes the list of methods and creates their slots and stub functions
void process_methods(const ast::cls& ast_cls, user_class* cls, cool_program& output, logger& log)
{
	llvm::Module* module = output.module();

	for (const ast::method& method : ast_cls.methods)
	{
		// Lookup types relating to this method
		cool_class* return_type = output.lookup_class(method.type);
		if (return_type == nullptr)
		{
			log.error(method.loc, "unknown type '" + method.type + "'");
			continue;
		}

		std::vector<cool_class*> parameter_types;
		for (auto& param_pair : method.params)
		{
			cool_class* type = output.lookup_class(param_pair.second);
			if (type == nullptr)
				log.error(method.loc, "unknown type '" + param_pair.second + "'");
			else
				parameter_types.push_back(type);
		}

		// Check if a method with this name already exists
		cool_method* existing_method = cls->lookup_method(method.name, true);
		if (existing_method == nullptr)
		{
			// Construct llvm function type
			std::vector<llvm::Type*> llvm_args;
			llvm_args.push_back(cls->llvm_type());
			for (cool_class* type : parameter_types)
				llvm_args.push_back(type->llvm_type());

			auto func_type = llvm::FunctionType::get(return_type->llvm_type(), llvm_args, false);

			// Create a new method slot
			auto slot = make_unique<cool_method_slot>();
			slot->name = method.name;
			slot->declaring_class = cls;
			slot->return_type = return_type;
			slot->parameter_types = std::move(parameter_types);

			// Create a stub function
			llvm::Function* func =
				create_fast_function(module, func_type, cls->name() + "." + method.name);

			// Add to list of methods
			cls->_methods.emplace(method.name, make_unique<cool_method>(std::move(slot), func));
		}
		else if (existing_method->declaring_class() == cls)
		{
			// Duplicate method
			log.error(method.loc, format("redefinition of method '%s'") % method.name);
		}
		else if (existing_method->slot()->return_type != return_type ||
		         existing_method->slot()->parameter_types != parameter_types)
		{
			// Method has incorrect signature
			log.error(method.loc, format("signature of method '%s' does not match inherited method from class '%s'")
				% method.name % existing_method->declaring_class()->name());
		}
		else
		{
			// Create a stub function using inherited method's type
			llvm::Function* func =
				create_fast_function(module, func_type, cls->name() + "." + method.name);

			// Add method override
			cls->_methods.emplace(method.name, make_unique<cool_method>(cls, existing_method, func));
		}
	}
}

// Creates an initializer for the vtable for the given class
//  top_cls     = the toplevel class this initializer will become part of
//  output      = output program
//  cls         = specific class initializer should be created for
llvm::Constant* create_partial_vtable_init(user_class* top_cls, cool_program& output, cool_class* cls)
{
	std::vector<llvm::Constant*> elements;

	// Object requires some (very) special handling
	if (cls->parent() == nullptr)
	{
		llvm::LLVMContext& context = output.module()->getContext();
		auto void_type = llvm::Type::getVoidTy(context);
		auto ptr_object_type = cls->llvm_type();

		// 0 Pointer to parent vtable
		elements.push_back(top_cls->parent()->llvm_vtable());

		// 1 Object size (calculated through some gep magic)
		auto null_ptr = llvm::ConstantPointerNull::get(top_cls->llvm_type());
		auto type_i32 = llvm::Type::getInt32Ty(output.module()->getContext());
		auto one = llvm::ConstantInt::get(type_i32, 1);
		auto gep_instruction = llvm::ConstantExpr::getGetElementPtr(null_ptr, one);
		auto final_size = llvm::ConstantExpr::getPtrToInt(gep_instruction, type_i32);
		elements.push_back(final_size);

		// 2 Pointer to name string
		elements.push_back(output.create_string_literal(top_cls->name(), top_cls->name() + "$name"));

		// 3 Constructor
		elements.push_back(create_fast_function(
			output.module(),
			llvm::FunctionType::get(void_type, ptr_object_type, false),
			top_cls->name() + "$construct"));

		// 4 Copy constructor
		std::vector<llvm::Type*> cc_func_params = { ptr_object_type, ptr_object_type };
		elements.push_back(create_fast_function(
			output.module(),
			llvm::FunctionType::get(void_type, cc_func_params, false),
			top_cls->name() + "$copyconstruct"));

		// 5 Destructor
		elements.push_back(create_fast_function(
			output.module(),
			llvm::FunctionType::get(void_type, ptr_object_type, false),
			top_cls->name() + "$destroy"));
	}
	else
	{
		// The first element is the parent class's vtable
		elements.push_back(create_partial_vtable_init(top_cls, output, cls->parent()));
	}

	// Handle all the other methods belonging to this class
	for (cool_method* method : cls->methods())
	{
		cool_method_slot* slot = method->slot();

		// We only care abound methods with new slots
		if (slot->declaring_class == cls)
		{
			// Lookup the method which this slot will resolve to at runtime
			auto resolved_method = top_cls->lookup_method(slot->name, true);
			assert(resolved_method != nullptr);
			assert(resolved_method->slot() == slot);

			// Add the correct function into the correct vtable slot
			if (slot->vtable_index >= static_cast<int>(elements.size()))
				elements.resize(slot->vtable_index + 1);

			elements[slot->vtable_index] = resolved_method->llvm_func();
		}
	}

	// Return final struct
	return llvm::ConstantStruct::getAnon(elements);
}

// Creates a class's vtable
void create_vtable(user_class* cls, cool_program& output)
{
	// The vtable's type is a struct containing the parent vtable struct and
	// function pointers for any methods we need to provide slots for.
	std::vector<llvm::Type*> type_elements;
	type_elements.push_back(cls->parent()->llvm_vtable()->getType());

	for (auto& method : cls->methods())
	{
		cool_method_slot* slot = method->slot();

		if (slot->declaring_class == cls)
		{
			// Add a slot for this method and record vtable index
			slot->vtable_index = type_elements.size();
			type_elements.push_back(method->llvm_func()->getType()->getPointerTo());
		}
	}

	// Recursively construct initializer for the vtable
	auto initializer = create_partial_vtable_init(cls, output, cls);

	// Create the vtable
	cls->_vtable = new llvm::GlobalVariable(
		*output.module(),
		initializer->getType(),
		true,
		llvm::GlobalVariable::InternalLinkage,
		initializer,
		cls->name() + "$vtable");
}

// Layout a single class
void layout_cls(const ast::cls& ast_cls, cool_program& output, logger& log)
{
	// Get cool_class instance
	user_class* cls = static_cast<user_class*>(output.lookup_class(ast_cls.name));
	assert(cls != nullptr);
	assert(cls->parent() != nullptr);

	// Process attributes + create the llvm struct
	process_attributes(ast_cls, cls, output, log);

	// Process methods
	process_methods(ast_cls, cls, output, log);

	// Create vtable + special functions
	create_vtable(cls, output);
}

} /* anonymous namespace */

void lcool::layout(const ast::program& input, cool_program& output, logger& log)
{
	// Basic sanity check
	assert(output.lookup_class("Object") != nullptr);

	// Insert empty versions of all classes
	auto layout_list = insert_empty_classes(input, output, log);

	if (!log.has_errors())
	{
		// Layout each of them in turn
		for (auto cls : layout_list)
			layout_cls(*cls, output, log);
	}
}
