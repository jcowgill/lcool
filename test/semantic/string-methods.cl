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

-- String methods tests

class Main inherits IO
{
	initial_str : String <- "foobar";
	string_sum : String;

	main() : Object
	{{
		-- Verify initial string lengths
		if not initial_str.length() = 6 then
			out_string("FAIL initial_str.length")
		else 0 fi;
		if not string_sum.length() = 0 then
			out_string("FAIL string_sum.length")
		else 0 fi;

		-- Append all substrings of initial_str to string_sum while checking
		-- lengths
		let start_pos : Int <- 0 in
		while start_pos < 6 loop
		{
			let length : Int <- 0 in
			while length <= (6 - start_pos) loop
			{
				let substr : String <- initial_str.substr(start_pos, length) in
				if not substr.length() = length then
					out_string("FAIL substr.length")
				else
					string_sum <- string_sum.concat(substr)
				fi;
				length <- length + 1;
			}
			pool;
			start_pos <- start_pos + 1;
		}
		pool;

		-- Print final string
		out_string(string_sum);
		out_string("\n");
	}};
};
