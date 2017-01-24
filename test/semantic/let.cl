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

-- Let statement tests

class Main inherits IO
{
	-- Print integer with newline
	out_int_ln(int : Int) : Object
	{{
		out_int(int);
		out_string("\n");
	}};

	main() : Object
	{{
		-- Basic sanity check
		let obj : Object <- "Test String 1\n" in
		case obj of
			str : String => out_string(str);
		esac;

		-- Nested variables
		let int1 : Int <- 1 in
		let int2 : Int <- int1 + 1 in
		{
			let int1 : Int <- 42 in out_int_ln(int1);
			out_int_ln(int1);
		};

		-- Multiple variables with interdependencies
		out_int_ln(
			let int1 : Int,
			    int2 : Int <- int1 + 1,
			    int1 : Int <- int1 + 2,
			    int2 : Int <- int1 + 1 in
			{
				out_int_ln(int1);
				out_int_ln(int2);
				int1 + int2;
			}
		);
	}};
};
