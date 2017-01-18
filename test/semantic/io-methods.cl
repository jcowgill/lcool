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

-- IO methods tests

class Main inherits IO
{
	echo_string() : Bool
	{
		let string : String <- in_string() in
		if string = "" then
			false
		else
		{
			out_string(string);
			out_string("\n");
			true;
		}
		fi
	};

	echo_int() : Bool
	{
		let int : Int <- in_int() in
		if int = 0 then
			false
		else
		{
			out_int(int * int);
			out_string("\n");
			true;
		}
		fi
	};

	main() : Object
	{{
		-- Print computed string and check return value
		if not out_string("Hello ".concat("world\n")) = self then
			abort()
		else 0 fi;

		-- Print number and check return value
		if not out_int(42) = self then
			abort()
		else
			out_string("\n")
		fi;

		-- Read and print strings/ints until EOF
		while (if echo_string() then echo_int() else false fi) loop 0 pool;
	}};
};
