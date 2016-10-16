(*
 * Copyright (C) 2016 James Cowgill
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

-- Comparison tests

class Main inherits IO
{
	-- Prints the result of a boolean value
	print_bool_test(test : String, value : Bool) : Object
	{{
		out_string(test);
		if value then
			out_string("True")
		else
			out_string("False")
		fi;
		out_string("\n");
	}};

	main() : Object
	{{
		print_bool_test("1 < 2 = ", 1 < 2);
		print_bool_test("1 < 1 = ", 1 < 1);
		print_bool_test("1 < 0 = ", 1 < 0);

		print_bool_test("1 <= 2 = ", 1 <= 2);
		print_bool_test("1 <= 1 = ", 1 <= 1);
		print_bool_test("1 <= 0 = ", 1 <= 0);

		print_bool_test("1 = 2 = ", 1 = 2);
		print_bool_test("1 = 1 = ", 1 = 1);
		print_bool_test("1 = 0 = ", 1 = 0);

		print_bool_test("-1 < 1 = ", 0-1 < 1);
		print_bool_test("-1 <= 1 = ", 0-1 <= 1);
		print_bool_test("-1 = 1 = ", 0-1 = 1);
	 }};
};
