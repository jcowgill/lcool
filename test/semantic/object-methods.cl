(*
 * Copyright (C) 2017 James Cowgill
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
 *)

-- Object methods tests

class Main inherits IO
{
	-- Create various objects to test
	io : Object <- new IO;
	io_copy : Object <- io.copy();
	this : Main <- self;
	this_copy : Main <- this.copy();

	get_io() : Object { io };

	println(str : String) : Object
	{{
		out_string(str);
		out_string("\n");
	}};

	main() : Object
	{{
		-- type_name on primitive types
		println(true.type_name());
		println(0.type_name());
		println("yo".type_name());

		-- Check object names
		println(type_name());
		println(io.type_name());
		println(io_copy.type_name());
		println(this.type_name());
		println(this_copy.type_name());

		-- Copies should be different
		if io = io_copy then println("FAIL io = io_copy") else 0 fi;
		if this = this_copy then println("FAIL this = this_copy") else 0 fi;

		-- Copies should be shallow
		if not this.get_io() = this_copy.get_io() then
			println("FAIL this.io = this_copy.io")
		else 0 fi;

		-- Check abort works
		this_copy.abort();
	}};
};
