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

-- Classes, inheritance, dispatch and case statements

class A
{
	override_c() : String { "A" };
	cls_name() : String { "A" };
};

class B inherits A
{
	cls_name() : String { "B" };
};

class C inherits B
{
	override_c() : String { "C" };
	cls_name() : String { "C" };
};

class D inherits A
{
	cls_name() : String { "D" };
};

class Main inherits IO
{
	-- Print class name using case
	class_to_name(obj : Object) : Object
	{
		out_string(case obj of
			o1 : Object => "O";
			o2 : A      => "A";
			o3 : C      => "C";
			o4 : B      => "B";
			o5 : D      => "D";
		esac)
	};

	main() : Object
	{{
		let dd : D <- new D,
		    cc : C <- new C,
		    bc : B <- cc,
		    bb : B <- new B,
		    ad : A <- dd,
		    ac : A <- bc,
		    ab : A <- bb,
		    aa : A <- new A in
		{
			-- Print classnames using dispatch + case methods
			out_string(dd.cls_name());
			out_string(cc.cls_name());
			out_string(bc.cls_name());
			out_string(bb.cls_name());
			out_string(ad.cls_name());
			out_string(ac.cls_name());
			out_string(ab.cls_name());
			out_string(aa.cls_name());
			out_string("\n");

			class_to_name(dd);
			class_to_name(cc);
			class_to_name(bc);
			class_to_name(bb);
			class_to_name(ad);
			class_to_name(ac);
			class_to_name(ab);
			class_to_name(aa);
			out_string("\n");

			-- Test case on some "other" objects
			class_to_name(1);
			class_to_name(new Object);
			class_to_name("cats are awesome");
			out_string("\n");

			-- Print using @A form (should just print As)
			out_string(dd@A.cls_name());
			out_string(cc@A.cls_name());
			out_string(bc@A.cls_name());
			out_string(bb@A.cls_name());
			out_string(ad@A.cls_name());
			out_string(ac@A.cls_name());
			out_string(ab@A.cls_name());
			out_string(aa@A.cls_name());
			out_string("\n");

			-- Method only overridden by C
			out_string(dd.override_c());
			out_string(cc.override_c());
			out_string(bb.override_c());
			out_string(aa.override_c());
			out_string("\n");
		};
	}};
};
