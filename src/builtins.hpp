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

#ifndef LCOOL_BUILTINS_HPP
#define LCOOL_BUILTINS_HPP

#include <llvm/IR/Module.h>
#include "smart_ptr.hpp"

namespace lcool
{
	class cool_program;

	/**
	 * Loads the lcool runtime into a new LLVM Module
	 *
	 * @param context LLVM context to creat Module in
	 */
	unique_ptr<llvm::Module> builtins_load_bitfile(llvm::LLVMContext& context);

	/**
	 * Registers the builtin classes into a cool_program
	 *
	 * The program's module must already contain the LLVM definitions for the
	 * builtin classes (which you can get using builtins_load_bitfile)
	 */
	void builtins_register(cool_program& program);
}

#endif
