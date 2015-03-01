/*
 * Copyright (C) 2015 James Cowgill
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

static void log_class_loop(insert_empty_classes_state& state, unsigned class_index)
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
static void insert_empty_classes_visit(insert_empty_classes_state& state, unsigned class_index)
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
		const cool_class* parent = state.output.lookup_class(parent_name);

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
		state.output.insert_class<cool_user_class>(cls.name, parent);

		// Add to layout list
		state.layout_list.push_back(&cls);
	}
}

// Inserts empty versions of classes into the output program
//  Returns a list of classes ordered such that each class's parent never
//  succeeds it in the list.
static std::vector<const ast::cls*>
	insert_empty_classes(
		const ast::program& input,
		cool_program& output,
		logger& log)
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

void lcool::layout(const ast::program& input, cool_program& output, logger& log)
{
	// Basic sanity check
	assert(output.lookup_class("Object") != nullptr);

	// Insert empty versions of all classes
	auto layout_list = insert_empty_classes(input, output, log);

	// DEBUG COMMENT OUT
#if 0
	if (!log.has_errors())
	{
		// Layout each of them in turn
		for (auto& item : layout_list)
			layout(item.first, item.second, output, log);
	}
#endif
}
