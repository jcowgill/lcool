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

-- Int / Bool boxing tests
-- Box constant / non-constant values
-- Store in attribute
-- Unbox - verify values are identical

class Main inherits IO
{
	boxed_zero : Object <- 0;
	boxed_one : Object;
	boxed_two : Object;

	boxed_false : Object <- false;
	boxed_reused : Object;

	-- Prints the result of a string, integer or boolean value
	print_test(test : String, value : Object) : Object
	{{
		out_string(test);
		case value of
			unboxed : Int => out_int(unboxed);
			unboxed : Bool => out_string(if unboxed then "True" else "False" fi);
			unboxed : String => out_string(unboxed);
		esac;
		out_string("\n");
	}};

	-- Unboxes an integer
	unbox_int(value : Object) : Int
	{
		case value of unboxed : Int => unboxed; esac
	};

	-- Unboxes a boolean
	unbox_bool(value : Object) : Bool
	{
		case value of unboxed : Bool => unboxed; esac
	};

	main() : Object
	{{
		-- Box a constant one
		boxed_one <- 1;

		-- Box a two depending on boxed values
		boxed_two <- case boxed_zero of
			unboxed : Int => unboxed + 2;
		esac;

		-- Verify values
		print_test("boxed_zero = ", boxed_zero);
		print_test("boxed_one = ", boxed_one);
		print_test("boxed_two = ", boxed_two);
		print_test("boxed_false = ", boxed_false);

		-- Box a boolean, verify and reuse it as an integer
		boxed_reused <- case boxed_false of
			unboxed : Bool => not unboxed;
		esac;
		print_test("boxed_reused (True) = ", boxed_reused);
		boxed_reused <- boxed_two;
		print_test("boxed_reused (2) = ", boxed_reused);
	}};
};
