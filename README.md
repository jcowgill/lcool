# LCool
LCool is a compiler for the Cool programming language that uses LLVM as its
backend.

## Compiling
You need boost with the "program options" library, and LLVM to compile lcool.

This project is written in C++ and uses CMake as its build system. To compile
and install it, run something like:

	mkdir build
	cd build
	cmake ..
	make
	make install

## LLVM
Currently lcool only works with LLVM 3.5. LLVM is detected using CMake's config
system. If you installed LLVM from your distribution's repository it should work
(eg on Debian install `llvm-3.5-dev`). If you compiled LLVM manually, you may
need to set the `-DCMAKE_PREFIX_PATH` variable to the root of your LLVM
installation.

## License
Copyright (C) 2014-2015 James Cowgill

LCool is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

LCool is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with LCool.  If not, see <http://www.gnu.org/licenses/>.
