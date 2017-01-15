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

-- Arithmetic tests

class Main inherits IO
{
	max_int : Int <- 2147483647;
	min_int : Int <- 0 - 2147483647 - 1;

	-- Prints the result of a boolean value
	print_int_test(test : String, value : Int) : Object
	{{
		out_string(test);
		out_int(value);
		out_string("\n");
	}};

	main() : Object
	{{
		-- Basic arithmetic
		print_int_test("1 + 1 = ", 1 + 1);
		print_int_test("1 + -1 = ", 1 + (0-1));
		print_int_test("1 * 1 = ", 1 * 1);
		print_int_test("1 * 0 = ", 1 * 0);
		print_int_test("1 / 1 = ", 1 / 1);
		print_int_test("10 * 42 = ", 10 * 42);
		print_int_test("-10 * 42 = ", (0-10) * 42);
		print_int_test("420 / 10 = ", 420 / 10);
		print_int_test("420 / -10 = ", 420 / (0-10));

		-- Large numbers + wraparound
		print_int_test("max_int + 1 = ", max_int + 1);
		print_int_test("min_int - 1 = ", min_int - 1);
		print_int_test("max_int * max_int = ", max_int * max_int);
		print_int_test("min_int * min_int = ", min_int * min_int);
		print_int_test("max_int / max_int = ", max_int / max_int);

		-- Negation
		print_int_test("~min_int = ", ~min_int);
		print_int_test("~0 = ", ~0);
		print_int_test("~42 = ", ~42);
		print_int_test("~(-42) = ", ~(0-42));
	}};
};
