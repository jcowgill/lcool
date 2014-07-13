; Copyright (C) 2014 James Cowgill
;
; LCool is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation, either version 3 of the License, or
; (at your option) any later version.
;
; LCool is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with LCool.  If not, see <http://www.gnu.org/licenses/>.

; Refcounting notes
; =================
; Refcounting is done using the standard rules used by other refcounting systems
; This is roughly as follows
;  Newly allocated objects have refcounts of 1
;  Refcounts are updated when objects are assigned to new places, rather than read
;  Objects used as inputs to functions do not need their refcounts updated
;  Objects returned from functions should have their refcounts incremented

; Object structures
%Object$vtabletype = type
{
	%Object$vtabletype*,  ; Pointer to parent vtable (null for Object)
	i32,                  ; Object size
	%String*,             ; Pointer to type_name of this class
	void (%Object*)*,     ; Destructor
	%Object* (%Object*)*, ; abort
	%Object* (%Object*)*, ; copy
	%String* (%Object*)*  ; type_name
}

%Object = type
{
	%Object$vtabletype*, ; Pointer to vtable
	i32                  ; Reference counter
}

%IO$vtabletype = type
{
	%Object$vtabletype,     ; Parent vtable
	i32 (%IO*)*,            ; in_int
	%String* (%IO*)*,       ; in_string
	%IO* (%IO*, i32)*,      ; out_int
	%IO* (%IO*, %String*)*  ; out_string
}

%IO = type { %Object }

%String = type
{
	%Object,   ; Parent type
	i32,       ; Length (bytes)
	[0 x i8]   ; String content
}

%Int = type
{
	%Object,   ; Parent type
	i32        ; Data
}

%Bool = type
{
	%Object,   ; Parent type
	i1         ; Data
}

; Type names
@Object$name = private global { %Object, i32, [6 x i8] }
	{ %Object { %Object$vtabletype* @String$vtable, i32 1 }, i32 6, [6 x i8] c"Object" }
@IO$name = private global { %Object, i32, [2 x i8] }
	{ %Object { %Object$vtabletype* @String$vtable, i32 1 }, i32 2, [2 x i8] c"IO" }
@String$name = private global { %Object, i32, [6 x i8] }
	{ %Object { %Object$vtabletype* @String$vtable, i32 1 }, i32 6, [6 x i8] c"String" }
@Int$name = private global { %Object, i32, [3 x i8] }
	{ %Object { %Object$vtabletype* @String$vtable, i32 1 }, i32 3, [3 x i8] c"Int" }
@Bool$name = private global { %Object, i32, [4 x i8] }
	{ %Object { %Object$vtabletype* @String$vtable, i32 1 }, i32 4, [4 x i8] c"Bool" }

; vtable instances
@Object$vtable = internal constant %Object$vtabletype
{
	%Object$vtabletype*  null,
	i32                  ptrtoint (%Object* getelementptr (%Object* null, i32 1) to i32),
	%String*             bitcast ({ %Object, i32, [6 x i8] }* @Object$name to %String*),
	void (%Object*)*     @Object$destroy,
	%Object* (%Object*)* @Object.abort,
	%Object* (%Object*)* @Object.copy,
	%String* (%Object*)* @Object.type_name
}

@IO$vtable = internal constant %IO$vtabletype
{
	%Object$vtabletype
	{
		%Object$vtabletype*  @Object$vtable,
		i32                  ptrtoint (%IO* getelementptr (%IO* null, i32 1) to i32),
		%String*             bitcast ({ %Object, i32, [2 x i8] }* @IO$name to %String*),
		void (%Object*)*     @Object$destroy,
		%Object* (%Object*)* @Object.abort,
		%Object* (%Object*)* @Object.copy,
		%String* (%Object*)* @Object.type_name
	},

	i32 (%IO*)*            @IO.in_int,
	%String* (%IO*)*       @IO.in_string,
	%IO* (%IO*, i32)*      @IO.out_int,
	%IO* (%IO*, %String*)* @IO.out_string
}

@String$vtable = internal constant %Object$vtabletype
{
	%Object$vtabletype*  @Object$vtable,
	i32                  ptrtoint (%String* getelementptr (%String* null, i32 1) to i32),
	%String*             bitcast ({ %Object, i32, [6 x i8] }* @String$name to %String*),
	void (%Object*)*     @Object$destroy,
	%Object* (%Object*)* @Object.abort,
	%Object* (%Object*)* @copy_noop,
	%String* (%Object*)* @Object.type_name
}

@Int$vtable = internal constant %Object$vtabletype
{
	%Object$vtabletype*  @Object$vtable,
	i32                  ptrtoint (%Int* getelementptr (%Int* null, i32 1) to i32),
	%String*             bitcast ({ %Object, i32, [3 x i8] }* @Int$name to %String*),
	void (%Object*)*     @Object$destroy,
	%Object* (%Object*)* @Object.abort,
	%Object* (%Object*)* @copy_noop,
	%String* (%Object*)* @Object.type_name
}

@Bool$vtable = internal constant %Object$vtabletype
{
	%Object$vtabletype*  @Object$vtable,
	i32                  ptrtoint (%Bool* getelementptr (%Bool* null, i32 1) to i32),
	%String*             bitcast ({ %Object, i32, [4 x i8] }* @Bool$name to %String*),
	void (%Object*)*     @Object$destroy,
	%Object* (%Object*)* @Object.abort,
	%Object* (%Object*)* @copy_noop,
	%String* (%Object*)* @Object.type_name
}

; The empty string
@String$empty = internal global %String
{
	%Object { %Object$vtabletype* @String$vtable, i32 1 },
	i32 0,
	[0 x i8] []
}

; External functions (from libc)
%IO$File = type opaque

declare void @abort() noreturn nounwind
declare noalias i8* @malloc(i32) nounwind
declare void @free(i8*) nounwind

declare i32 @printf(i8*, ...) nounwind
declare i32 @puts(i8*) nounwind
declare i8* @fgets(i8*, i32, %IO$File*) nounwind
declare i32 @fflush(%IO$File*) nounwind
declare i32 @atoi(i8*) nounwind
declare i32 @strlen(i8*) nounwind

declare void @llvm.memcpy.p0i8.p0i8.i32(i8*, i8*, i32, i32, i1)

@stdout = external global %IO$File*
@stdin = external global %IO$File*

; Internal string table
@format_str = private unnamed_addr constant [5 x i8] c"%.*s\00"
@format_int = private unnamed_addr constant [3 x i8] c"%d\00"
@err_oom = private unnamed_addr constant [14 x i8] c"Out of memory\00"
@err_abort = private unnamed_addr constant [22 x i8] c"Object.abort() called\00"
@err_null_strlen = private unnamed_addr constant [38 x i8] c"String.length() called on void string\00"
@err_null_strcat = private unnamed_addr constant [38 x i8] c"String.concat() called on void string\00"
@err_null_strsub = private unnamed_addr constant [38 x i8] c"String.substr() called on void string\00"
@err_null_generic = private unnamed_addr constant [27 x i8] c"Illegal use of void object\00"
@err_range = private unnamed_addr constant [35 x i8] c"Bad range for String.substr() call\00"

; Builtin method implementations

; Prints a message and calls abort
define internal fastcc void @abort_with_msg(i8* %msg) noreturn
{
	call i32 @puts(i8* %msg)
	%stdout = load %IO$File** @stdout
	call i32 @fflush(%IO$File* %stdout)
	call void @abort()
	unreachable
}

; Allocates space for an object and sets its vtable pointer
define internal fastcc %Object* @alloc_object(%Object$vtabletype* %vtable)
{
	; Get size and call alloc_object_with_size
	%size_ptr = getelementptr %Object$vtabletype* %vtable, i32 0, i32 1
	%size = load i32* %size_ptr

	%result = tail call fastcc %Object* @alloc_object_with_size(i32 %size, %Object$vtabletype* %vtable)
	ret %Object* %result
}

; Like alloc_object but size is manually specified
define internal fastcc %Object* @alloc_object_with_size(i32 %size, %Object$vtabletype* %vtable)
{
	; Allocate space for object
	%ptr = call i8* @malloc(i32 %size)

	; Test if result was null
	%ptr_is_null = icmp eq i8* %ptr, null
	br i1 %ptr_is_null, label %Null, label %NotNull

NotNull:
	; Initialize object and return
	%ptr_as_object = bitcast i8* %ptr to %Object*

	%vtable_ptr = getelementptr %Object* %ptr_as_object, i32 0, i32 0
	store %Object$vtabletype* %vtable, %Object$vtabletype** %vtable_ptr

	%refcount_ptr = getelementptr %Object* %ptr_as_object, i32 0, i32 1
	store i32 1, i32* %refcount_ptr

	ret %Object* %ptr_as_object

Null:
	; Abort out of memory
	call fastcc void @abort_with_msg(i8* getelementptr ([14 x i8]* @err_oom, i32 0, i32 0))
	unreachable
}

; Allocate a string (uninitialized)
define internal fastcc %String* @alloc_string(i32 %size)
{
	; Get size of new object
	%empty_size = ptrtoint %String* getelementptr (%String* null, i32 1) to i32
	%obj_size = add nuw i32 %size, %empty_size

	; Allocate object
	%obj = call fastcc %Object* @alloc_object_with_size(i32 %obj_size, %Object$vtabletype* @String$vtable)
	%str = bitcast %Object* %obj to %String*

	; Store size of string
	%len_ptr = getelementptr %String* %str, i32 0, i32 1
	store i32 %size, i32* %len_ptr
	ret %String* %str
}

; Fast version of copy for immutable objects
define internal fastcc %Object* @copy_noop(%Object* %this)
{
	call fastcc void @refcount_inc(%Object* %this)
	ret %Object* %this
}

; Determines if an object is an instance of the given class (or a subclass)
define internal fastcc i1 @instance_of(%Object* %this, %Object$vtabletype* %cls) readonly
{
	; Get object vtable
	%vtable1ptr = getelementptr %Object* %this, i32 0, i32 0
	%vtable1 = load %Object$vtabletype** %vtable1ptr
	br label %LoopStart

LoopStart:
	; Fail with no instance if the current vtable is null
	%current_vtable = phi %Object$vtabletype* [ %vtable1, %0 ], [ %next_vtable, %LoopCheckCls ]
	%is_null = icmp eq %Object$vtabletype* %current_vtable, null
	br i1 %is_null, label %NoInstance, label %LoopCheckCls

LoopCheckCls:
	; Get next vtable to check
	%next_vtable_ptr = getelementptr %Object$vtabletype* %current_vtable, i32 0, i32 0
	%next_vtable = load %Object$vtabletype** %next_vtable_ptr

	; Pass if the current vtable is the one we want
	%is_the_one = icmp eq %Object$vtabletype* %current_vtable, %cls
	br i1 %is_the_one, label %GoodInstance, label %LoopStart

GoodInstance:
	ret i1 1

NoInstance:
	ret i1 0
}

; Verifys that the given argument is not null
define internal fastcc void @null_check(%Object* %this) inlinehint
{
	%is_null = icmp eq %Object* %this, null
	br i1 %is_null, label %Null, label %NotNull

NotNull:
	ret void

Null:
	call fastcc void @abort_with_msg(i8* getelementptr ([27 x i8]* @err_null_generic, i32 0, i32 0))
	unreachable
}

; Increments the refcount on an object
define internal fastcc void @refcount_inc(%Object* %this) inlinehint
{
	%refcount_ptr = getelementptr %Object* %this, i32 0, i32 1
	%refcount_old = load i32* %refcount_ptr
	%refcount_new = add nuw i32 %refcount_old, 1
	store i32 %refcount_new, i32* %refcount_ptr
	ret void
}

; Decrements the refcount on an object
define internal fastcc void @refcount_dec(%Object* %this) inlinehint
{
	; Get refcount and see if we should destroy it
	%refcount_ptr = getelementptr %Object* %this, i32 0, i32 1
	%refcount_old = load i32* %refcount_ptr
	%is_garbage = icmp ule i32 %refcount_old, 1
	br i1 %is_garbage, label %Garbage, label %Decrement

Garbage:
	; Call its destructor
	%vtable_ptr = getelementptr %Object* %this, i32 0, i32 0
	%vtable = load %Object$vtabletype** %vtable_ptr
	%destroy_ptr = getelementptr %Object$vtabletype* %vtable, i32 0, i32 3
	%destroy = load void (%Object*)** %destroy_ptr
	tail call fastcc void %destroy(%Object* %this)
	ret void

Decrement:
	; Decrement counter
	%refcount_new = sub nuw i32 %refcount_old, 1
	store i32 %refcount_new, i32* %refcount_ptr
	ret void
}

; Destroy the given object
define internal fastcc void @Object$destroy(%Object* %this)
{
	%this_as_i8 = bitcast %Object* %this to i8*
	call void @free(i8* %this_as_i8)
	ret void
}

; Prints an error message and calls abort()
define internal fastcc %Object* @Object.abort(%Object* %this) noreturn
{
	call fastcc void @abort_with_msg(i8* getelementptr ([22 x i8]* @err_abort, i32 0, i32 0))
	unreachable
}

; Copies the given object
define internal fastcc %Object* @Object.copy(%Object* %this)
{
	; Allocate new object with same size as %this and return it
	%vtable_ptr = getelementptr %Object* %this, i32 0, i32 0
	%vtable = load %Object$vtabletype** %vtable_ptr

	%new = tail call fastcc %Object* @alloc_object(%Object$vtabletype* %vtable)
	ret %Object* %new
}

; Returns the name of the type of an object
define internal fastcc %String* @Object.type_name(%Object* %this)
{
	; Extract type_name from vtable
	%vtable_ptr = getelementptr %Object* %this, i32 0, i32 0
	%vtable = load %Object$vtabletype** %vtable_ptr

	%type_name_ptr = getelementptr %Object$vtabletype* %vtable, i32 0, i32 2
	%type_name = load %String** %type_name_ptr

	; Increment refcount on string
	%type_name_obj = getelementptr %String* %type_name, i32 0, i32 0
	call fastcc void @refcount_inc(%Object* %type_name_obj)

	; Return final string
	ret %String* %type_name
}

; Prints a string (returns self)
define internal fastcc %IO* @IO.out_string(%IO* %this, %String* %value)
{
	; Get string length and data pointer
	%str_len = call fastcc i32 @String.length(%String* %value)
	%str_data = getelementptr %String* %value, i32 0, i32 2, i32 0

	; Call printf
	%format = getelementptr [5 x i8]* @format_str, i32 0, i32 0
	call i32 (i8*, ...)* @printf(i8* %format, i32 %str_len, i8* %str_data)

	; Return this
	%this_obj = getelementptr %IO* %this, i32 0, i32 0
	call fastcc void @refcount_inc(%Object* %this_obj)
	ret %IO* %this
}

; Prints an integer (returns self)
define internal fastcc %IO* @IO.out_int(%IO* %this, i32 %value)
{
	; Call printf
	%format = getelementptr [3 x i8]* @format_int, i32 0, i32 0
	call i32 (i8*, ...)* @printf(i8* %format, i32 %value)

	; Return this
	%this_obj = getelementptr %IO* %this, i32 0, i32 0
	call fastcc void @refcount_inc(%Object* %this_obj)
	ret %IO* %this
}

; Reads one line of input (does not read new line character)
define internal fastcc %String* @IO.in_string(%IO* %this)
{
	%buf = alloca i8, i32 4096

	; Run fgets
	%stdin = load %IO$File** @stdin
	%fgets_result = call i8* @fgets(i8* %buf, i32 4096, %IO$File* %stdin)

	; Check for EOF
	%is_eof = icmp eq i8* %fgets_result, null
	br i1 %is_eof, label %Eof, label %MakeString

MakeString:
	; Get string size
	%orig_str_len = call i32 @strlen(i8* %buf)

	; Erase \n if it is at the end
	%newline_pos = sub nuw i32 %orig_str_len, 1
	%newline_ptr = getelementptr i8* %buf, i32 %newline_pos
	%newline_char = load i8* %newline_ptr
	%is_newline = icmp eq i8 %newline_char, 10

	%str_len = select i1 %is_newline, i32 %newline_pos, i32 %orig_str_len

	; Allocate string object
	%str = call fastcc %String* @alloc_string(i32 %str_len)

	; Copy string data
	%str_data_ptr = getelementptr %String* %str, i32 0, i32 2, i32 0
	call void @llvm.memcpy.p0i8.p0i8.i32(i8* %str_data_ptr, i8* %buf, i32 %str_len, i32 0, i1 0)

	ret %String* %str

Eof:
	; Return the empty string
	%empty_object = getelementptr %String* @String$empty, i32 0, i32 0
	call fastcc void @refcount_inc(%Object* %empty_object)
	ret %String* @String$empty
}

; Reads one line of input and parses it as an integer
define internal fastcc i32 @IO.in_int(%IO* %this)
{
	%buf = alloca i8, i32 4096

	; Run fgets
	%stdin = load %IO$File** @stdin
	%fgets_result = call i8* @fgets(i8* %buf, i32 4096, %IO$File* %stdin)

	; Check for EOF
	%is_eof = icmp eq i8* %fgets_result, null
	br i1 %is_eof, label %Eof, label %MakeInt

MakeInt:
	; Run atoi to parse integer
	%result = call i32 @atoi(i8* %buf)
	ret i32 %result

Eof:
	ret i32 0
}

; Boxes an integer into an object
define internal fastcc %Object* @Int$box(i32 %value)
{
	; Allocate integer object
	%new = call fastcc %Object* @alloc_object(%Object$vtabletype* @Int$vtable)

	; Store value into it
	%new_int = bitcast %Object* %new to %Int*
	%value_ptr = getelementptr %Int* %new_int, i32 0, i32 1
	store i32 %value, i32* %value_ptr

	ret %Object* %new
}

; Boxes a boolean into an object
define internal fastcc %Object* @Bool$box(i1 %value)
{
	; Allocate boolean object
	%new = call fastcc %Object* @alloc_object(%Object$vtabletype* @Bool$vtable)

	; Store value into it
	%new_bool = bitcast %Object* %new to %Bool*
	%value_ptr = getelementptr %Bool* %new_bool, i32 0, i32 1
	store i1 %value, i1* %value_ptr

	ret %Object* %new
}

; Returns the string's length
define internal fastcc i32 @String.length(%String* %this) inlinehint
{
	; Test for null string
	;  This is done for strings since this method is called statically
	%is_null = icmp eq %String* %this, null
	br i1 %is_null, label %Null, label %NotNull

NotNull:
	%length_ptr = getelementptr %String* %this, i32 0, i32 1
	%length = load i32* %length_ptr
	ret i32 %length

Null:
	call fastcc void @abort_with_msg(i8* getelementptr ([38 x i8]* @err_null_strlen, i32 0, i32 0))
	unreachable
}

; Concatenates two strings
define internal fastcc %String* @String.concat(%String* %this, %String* %other)
{
	; Test if any inputs are null
	%this_is_null  = icmp eq %String* %this, null
	%other_is_null = icmp eq %String* %other, null
	%any_are_null  = or i1 %this_is_null, %other_is_null
	br i1 %any_are_null, label %Null, label %NotNull

NotNull:
	; Get length of new string
	%this_len_ptr = getelementptr %String* %this, i32 0, i32 1
	%this_len = load i32* %this_len_ptr
	%other_len_ptr = getelementptr %String* %other, i32 0, i32 1
	%other_len = load i32* %other_len_ptr
	%new_len = add nuw i32 %this_len, %other_len

	; Allocate string
	%new = call fastcc %String* @alloc_string(i32 %new_len)

	; Calculate data pointers
	%new_data_ptr   = getelementptr %String* %new, i32 0, i32 2, i32 0
	%new_data_ptr2  = getelementptr i8* %new_data_ptr, i32 %this_len
	%this_data_ptr  = getelementptr %String* %this, i32 0, i32 2, i32 0
	%other_data_ptr = getelementptr %String* %other, i32 0, i32 2, i32 0

	; Copy data
	call void @llvm.memcpy.p0i8.p0i8.i32(i8* %new_data_ptr,  i8* %this_data_ptr,  i32 %this_len,  i32 0, i1 0)
	call void @llvm.memcpy.p0i8.p0i8.i32(i8* %new_data_ptr2, i8* %other_data_ptr, i32 %other_len, i32 0, i1 0)

	ret %String* %new

Null:
	call fastcc void @abort_with_msg(i8* getelementptr ([38 x i8]* @err_null_strcat, i32 0, i32 0))
	unreachable
}

; Extracts a substring of this string
define internal fastcc %String* @String.substr(%String* %this, i32 %i, i32 %l)
{
	; Test for null string
	%is_null = icmp eq %String* %this, null
	br i1 %is_null, label %Null, label %NotNull

NotNull:
	; Get original string length
	%this_len_ptr = getelementptr %String* %this, i32 0, i32 1
	%this_len = load i32* %this_len_ptr

	; Test for various bounds and special cases
	%is_empty = icmp eq i32 %l, 0
	br i1 %is_empty, label %Empty, label %Bounds1
Bounds1:
	%is_l_too_big = icmp ugt i32 %l, %this_len
	br i1 %is_l_too_big, label %RangeError, label %Bounds2
Bounds2:
	%len_sub_l = sub nuw i32 %this_len, %l
	%is_i_too_big = icmp ugt i32 %i, %len_sub_l
	br i1 %is_i_too_big, label %RangeError, label %Bounds3
Bounds3:
	%is_self = icmp eq i32 %len_sub_l, 0
	br i1 %is_self, label %Self, label %Normal

Normal:
	; Normal substring operation - allocate and copy data
	%new = call fastcc %String* @alloc_string(i32 %l)
	%new_data_ptr = getelementptr %String* %new, i32 0, i32 2, i32 0
	%old_data_ptr = getelementptr %String* %this, i32 0, i32 2, i32 %i
	call void @llvm.memcpy.p0i8.p0i8.i32(i8* %new_data_ptr, i8* %old_data_ptr, i32 %l, i32 0, i1 0)
	ret %String* %new

Self:
	; Return this with no changes
	%this_object = getelementptr %String* %this, i32 0, i32 0
	call fastcc void @refcount_inc(%Object* %this_object)
	ret %String* %this

Empty:
	; Return the empty string
	%empty_object = getelementptr %String* @String$empty, i32 0, i32 0
	call fastcc void @refcount_inc(%Object* %empty_object)
	ret %String* @String$empty

Null:
	call fastcc void @abort_with_msg(i8* getelementptr ([38 x i8]* @err_null_strsub, i32 0, i32 0))
	unreachable

RangeError:
	call fastcc void @abort_with_msg(i8* getelementptr ([35 x i8]* @err_range, i32 0, i32 0))
	unreachable
}