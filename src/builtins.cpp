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
#include <llvm/Linker.h>
#include <llvm/Support/MemoryBuffer.h>
#include <cstdlib>
#include <iostream>
#include <string>

#include "builtins.hpp"
#include "program_structure.hpp"

using lcool::unique_ptr;

using llvm::Linker;
using llvm::MemoryBuffer;
using llvm::Module;
using llvm::ParseBitcodeFile;
using llvm::StringRef;

// LLVM builtins bitcode file
namespace
{
	const unsigned char bitcode_data[] = {
#include "lcool_runtime.inc"
	};
}

// Link the runtime file into the given module
static void link_runtime(Module* dest)
{
	// Parse bitcode_data to get an LLVM module
	StringRef bitcode_data_ref(reinterpret_cast<const char*>(bitcode_data), sizeof(bitcode_data));
	unique_ptr<MemoryBuffer> buf(MemoryBuffer::getMemBuffer(bitcode_data_ref, "", false));
	Module* src = ParseBitcodeFile(buf.get(), dest->getContext());

	// Link module into main program
	if (src == nullptr || !Linker::LinkModules(dest, src, Linker::DestroySource, nullptr))
	{
		// Failed to link runtime (should never happen)
		std::cerr << "fatal error: failed to load lcool runtime bitcode file" << std::endl;
		std::abort();
	}
}

void lcool::load_builtins(lcool::program_structure& program)
{
	// Link in runtime library
	link_runtime(&program.llvm_module());
}
