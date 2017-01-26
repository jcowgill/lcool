; Copyright (C) 2014-2015 James Cowgill
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

; Public types list
; =================
; Object$vtabletype
; Object
; IO$vtabletype
; IO
; String
; Int
; Bool

; Public constant list (rodata)
; =============================
; Object$vtable  (Object$vtabletype)
; IO$vtable      (IO$vtabletype)
; String$vtable  (Object$vtabletype)
; Int$vtable     (Object$vtabletype)
; Bool$vtable    (Object$vtabletype)

; Public variable list (data)
; ===========================
; String$empty   (String)

; Public function list
; ====================
; (all Object and IO functions are called through a vtable)
;
; instance_of
; new_object
; null_check
; refcount_inc
; refcount_dec
; zero_division_check
;
; Int$box
; Bool$box
;
; String.length
; String.concat
; String.substr
; String$equals

; Object structures
%Object$vtabletype = type
{
	%Object$vtabletype*,       ; Pointer to parent vtable (null for Object)
	i32,                       ; Object size
	%String*,                  ; Pointer to type_name of this class
	void (%Object*)*,          ; Constructor
	void (%Object*, %Object*)*,; Copy constructor
	void (%Object*)*,          ; Destructor
	%Object* (%Object*)*,      ; abort
	%Object* (%Object*)*,      ; copy
	%String* (%Object*)*       ; type_name
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
@Object$vtable = hidden constant %Object$vtabletype
{
	%Object$vtabletype*        null,
	i32                        ptrtoint (%Object* getelementptr (%Object, %Object* null, i32 1) to i32),
	%String*                   bitcast ({ %Object, i32, [6 x i8] }* @Object$name to %String*),
	void (%Object*)*           @noop_construct,
	void (%Object*, %Object*)* @noop_copyconstruct,
	void (%Object*)*           @Object$destroy,
	%Object* (%Object*)*       @Object.abort,
	%Object* (%Object*)*       @Object.copy,
	%String* (%Object*)*       @Object.type_name
}

@IO$vtable = hidden constant %IO$vtabletype
{
	%Object$vtabletype
	{
		%Object$vtabletype*        @Object$vtable,
		i32                        ptrtoint (%IO* getelementptr (%IO, %IO* null, i32 1) to i32),
		%String*                   bitcast ({ %Object, i32, [2 x i8] }* @IO$name to %String*),
		void (%Object*)*           @noop_construct,
		void (%Object*, %Object*)* @noop_copyconstruct,
		void (%Object*)*           @Object$destroy,
		%Object* (%Object*)*       @Object.abort,
		%Object* (%Object*)*       @Object.copy,
		%String* (%Object*)*       @Object.type_name
	},

	i32 (%IO*)*            @IO.in_int,
	%String* (%IO*)*       @IO.in_string,
	%IO* (%IO*, i32)*      @IO.out_int,
	%IO* (%IO*, %String*)* @IO.out_string
}

@String$vtable = hidden constant %Object$vtabletype
{
	%Object$vtabletype*        @Object$vtable,
	i32                        ptrtoint (%String* getelementptr (%String, %String* null, i32 1) to i32),
	%String*                   bitcast ({ %Object, i32, [6 x i8] }* @String$name to %String*),
	void (%Object*)*           @noop_construct,
	void (%Object*, %Object*)* @noop_copyconstruct,
	void (%Object*)*           @Object$destroy,
	%Object* (%Object*)*       @Object.abort,
	%Object* (%Object*)*       @Object.copy,
	%String* (%Object*)*       @Object.type_name
}

@Int$vtable = hidden constant %Object$vtabletype
{
	%Object$vtabletype*        @Object$vtable,
	i32                        ptrtoint (%Int* getelementptr (%Int, %Int* null, i32 1) to i32),
	%String*                   bitcast ({ %Object, i32, [3 x i8] }* @Int$name to %String*),
	void (%Object*)*           @noop_construct,
	void (%Object*, %Object*)* @noop_copyconstruct,
	void (%Object*)*           @Object$destroy,
	%Object* (%Object*)*       @Object.abort,
	%Object* (%Object*)*       @Object.copy,
	%String* (%Object*)*       @Object.type_name
}

@Bool$vtable = hidden constant %Object$vtabletype
{
	%Object$vtabletype*        @Object$vtable,
	i32                        ptrtoint (%Bool* getelementptr (%Bool, %Bool* null, i32 1) to i32),
	%String*                   bitcast ({ %Object, i32, [4 x i8] }* @Bool$name to %String*),
	void (%Object*)*           @noop_construct,
	void (%Object*, %Object*)* @noop_copyconstruct,
	void (%Object*)*           @Object$destroy,
	%Object* (%Object*)*       @Object.abort,
	%Object* (%Object*)*       @Object.copy,
	%String* (%Object*)*       @Object.type_name
}

; The empty string
@String$empty = hidden global %String
{
	%Object { %Object$vtabletype* @String$vtable, i32 1 },
	i32 0,
	[0 x i8] []
}

; External functions (from libc)
%IO$File = type opaque

declare void @exit(i32) noreturn nounwind
declare noalias i8* @malloc(i32) nounwind
declare void @free(i8*) nounwind

declare i32 @printf(i8*, ...) nounwind
declare i32 @fputs(i8*, %IO$File*) nounwind
declare i32 @fputc(i32, %IO$File*) nounwind
declare i8* @fgets(i8*, i32, %IO$File*) nounwind
declare i32 @fflush(%IO$File*) nounwind
declare i32 @atoi(i8*) nounwind
declare i32 @strlen(i8*) nounwind

declare void @llvm.memcpy.p0i8.p0i8.i32(i8*, i8*, i32, i32, i1)

@stderr = external global %IO$File*
@stdout = external global %IO$File*
@stdin = external global %IO$File*

; Internal string table
@format_str = private unnamed_addr constant [5 x i8] c"%.*s\00"
@format_int = private unnamed_addr constant [3 x i8] c"%d\00"
@err_oom = private unnamed_addr constant [14 x i8] c"Out of memory\00"
@err_abort = private unnamed_addr constant [22 x i8] c"Object.abort() called\00"
@err_null = private unnamed_addr constant [27 x i8] c"Illegal use of void object\00"
@err_range = private unnamed_addr constant [35 x i8] c"Bad range for String.substr() call\00"
@err_div_zero = private unnamed_addr constant [17 x i8] c"Division by zero\00"

; Builtin method implementations

; Prints a message and exits
define hidden fastcc void @abort_with_msg(i8* %msg) noreturn
{
	%stdout = load %IO$File*, %IO$File** @stdout
	%stderr = load %IO$File*, %IO$File** @stderr
	call i32 @fflush(%IO$File* %stdout)
	call i32 @fputs(i8* %msg, %IO$File* %stderr)
	call i32 @fputc(i32 10, %IO$File* %stderr)
	call void @exit(i32 1)
	unreachable
}

; Allocates space for an object and sets its vtable pointer
define hidden fastcc %Object* @alloc_object(%Object$vtabletype* %vtable)
{
	; Get size and call alloc_object_with_size
	%size_ptr = getelementptr inbounds %Object$vtabletype, %Object$vtabletype* %vtable, i32 0, i32 1
	%size = load i32, i32* %size_ptr

	%result = tail call fastcc %Object* @alloc_object_with_size(i32 %size, %Object$vtabletype* %vtable)
	ret %Object* %result
}

; Like alloc_object but size is manually specified
define hidden fastcc %Object* @alloc_object_with_size(i32 %size, %Object$vtabletype* %vtable)
{
	; Allocate space for object
	%ptr = call i8* @malloc(i32 %size)

	; Test if result was null
	%ptr_is_null = icmp eq i8* %ptr, null
	br i1 %ptr_is_null, label %Null, label %NotNull

NotNull:
	; Initialize object and return
	%ptr_as_object = bitcast i8* %ptr to %Object*

	%vtable_ptr = getelementptr inbounds %Object, %Object* %ptr_as_object, i32 0, i32 0
	store %Object$vtabletype* %vtable, %Object$vtabletype** %vtable_ptr

	%refcount_ptr = getelementptr inbounds %Object, %Object* %ptr_as_object, i32 0, i32 1
	store i32 1, i32* %refcount_ptr

	ret %Object* %ptr_as_object

Null:
	; Abort out of memory
	call fastcc void @abort_with_msg(i8* getelementptr inbounds ([14 x i8], [14 x i8]* @err_oom, i32 0, i32 0))
	unreachable
}

; Allocate a string (uninitialized)
define hidden fastcc %String* @alloc_string(i32 %size)
{
	; If size is 0, just return the empty string
	%is_empty = icmp eq i32 %size, 0
	br i1 %is_empty, label %Empty, label %DoAllocate

DoAllocate:
	; Get size of new object
	%empty_size = ptrtoint %String* getelementptr (%String, %String* null, i32 1) to i32
	%obj_size = add nuw i32 %size, %empty_size

	; Allocate object
	%obj = call fastcc %Object* @alloc_object_with_size(i32 %obj_size, %Object$vtabletype* @String$vtable)
	%str = bitcast %Object* %obj to %String*

	; Store size of string
	%len_ptr = getelementptr inbounds %String, %String* %str, i32 0, i32 1
	store i32 %size, i32* %len_ptr
	ret %String* %str

Empty:
	; Return the empty string
	%empty_object = getelementptr inbounds %String, %String* @String$empty, i32 0, i32 0
	call fastcc void @refcount_inc(%Object* %empty_object)
	ret %String* @String$empty
}

; Determines if an object is an instance of the given class (or a subclass)
define hidden fastcc i1 @instance_of(%Object* %this, %Object$vtabletype* %cls) readonly
{
	; Get object vtable
	%vtable1ptr = getelementptr inbounds %Object, %Object* %this, i32 0, i32 0
	%vtable1 = load %Object$vtabletype*, %Object$vtabletype** %vtable1ptr
	br label %LoopStart

LoopStart:
	; Fail with no instance if the current vtable is null
	%current_vtable = phi %Object$vtabletype* [ %vtable1, %0 ], [ %next_vtable, %LoopCheckCls ]
	%is_null = icmp eq %Object$vtabletype* %current_vtable, null
	br i1 %is_null, label %NoInstance, label %LoopCheckCls

LoopCheckCls:
	; Get next vtable to check
	%next_vtable_ptr = getelementptr inbounds %Object$vtabletype, %Object$vtabletype* %current_vtable, i32 0, i32 0
	%next_vtable = load %Object$vtabletype*, %Object$vtabletype** %next_vtable_ptr

	; Pass if the current vtable is the one we want
	%is_the_one = icmp eq %Object$vtabletype* %current_vtable, %cls
	br i1 %is_the_one, label %GoodInstance, label %LoopStart

GoodInstance:
	ret i1 1

NoInstance:
	ret i1 0
}

; Allocates and initializes a new object of the given type
define hidden fastcc %Object* @new_object(%Object$vtabletype* %vtable)
{
	; Allocate object
	%new = call fastcc %Object* @alloc_object(%Object$vtabletype* %vtable)

	; Call constructor
	%construct_ptr = getelementptr inbounds %Object$vtabletype, %Object$vtabletype* %vtable, i32 0, i32 3
	%construct = load void (%Object*)*, void (%Object*)** %construct_ptr
	call fastcc void %construct(%Object* %new)
	ret %Object* %new
}

; Do nothing constructor
define hidden fastcc void @noop_construct(%Object*)
{
	ret void
}

; Do nothing copy constructor
define hidden fastcc void @noop_copyconstruct(%Object*, %Object*)
{
	ret void
}

; Verifys that the given argument is not null
define hidden fastcc void @null_check(%Object* %this) inlinehint
{
	%is_null = icmp eq %Object* %this, null
	br i1 %is_null, label %Null, label %NotNull

NotNull:
	ret void

Null:
	call fastcc void @abort_with_msg(i8* getelementptr inbounds ([27 x i8], [27 x i8]* @err_null, i32 0, i32 0))
	unreachable
}

; Increments the refcount on an object
define hidden fastcc void @refcount_inc(%Object* %this) inlinehint
{
	; Check for null objects
	%is_null = icmp eq %Object* %this, null
	br i1 %is_null, label %Null, label %NotNull

NotNull:
	%refcount_ptr = getelementptr inbounds %Object, %Object* %this, i32 0, i32 1
	%refcount_old = load i32, i32* %refcount_ptr
	%refcount_new = add nuw i32 %refcount_old, 1
	store i32 %refcount_new, i32* %refcount_ptr
	ret void

Null:
	ret void
}

; Decrements the refcount on an object
define hidden fastcc void @refcount_dec(%Object* %this) inlinehint
{
	; Check for null objects
	%is_null = icmp eq %Object* %this, null
	br i1 %is_null, label %Null, label %NotNull

NotNull:
	; Get refcount and see if we should destroy it
	%refcount_ptr = getelementptr inbounds %Object, %Object* %this, i32 0, i32 1
	%refcount_old = load i32, i32* %refcount_ptr
	%is_garbage = icmp ule i32 %refcount_old, 1
	br i1 %is_garbage, label %Garbage, label %Decrement

Garbage:
	; Call its destructor
	%vtable_ptr = getelementptr inbounds %Object, %Object* %this, i32 0, i32 0
	%vtable = load %Object$vtabletype*, %Object$vtabletype** %vtable_ptr
	%destroy_ptr = getelementptr inbounds %Object$vtabletype, %Object$vtabletype* %vtable, i32 0, i32 5
	%destroy = load void (%Object*)*, void (%Object*)** %destroy_ptr
	tail call fastcc void %destroy(%Object* %this)
	ret void

Decrement:
	; Decrement counter
	%refcount_new = sub nuw i32 %refcount_old, 1
	store i32 %refcount_new, i32* %refcount_ptr
	ret void

Null:
	; Do nothing
	ret void
}

; Verifies that the given argument is not 0
define hidden fastcc void @zero_division_check(i32 %divisor) inlinehint
{
	%is_zero = icmp eq i32 %divisor, 0
	br i1 %is_zero, label %Zero, label %NotZero

NotZero:
	ret void

Zero:
	call fastcc void @abort_with_msg(i8* getelementptr inbounds ([17 x i8], [17 x i8]* @err_div_zero, i32 0, i32 0))
	unreachable
}

; Destroy the given object
define hidden fastcc void @Object$destroy(%Object* %this)
{
	%this_as_i8 = bitcast %Object* %this to i8*
	call void @free(i8* %this_as_i8)
	ret void
}

; Prints an error message and calls abort()
define hidden fastcc %Object* @Object.abort(%Object* %this) noreturn
{
	call fastcc void @abort_with_msg(i8* getelementptr inbounds ([22 x i8], [22 x i8]* @err_abort, i32 0, i32 0))
	unreachable
}

; Copies the given object
define hidden fastcc %Object* @Object.copy(%Object* %this)
{
	; Get the vtable
	%vtable_ptr = getelementptr inbounds %Object, %Object* %this, i32 0, i32 0
	%vtable = load %Object$vtabletype*, %Object$vtabletype** %vtable_ptr

	; Strings, Ints and Bools would require special handling but since they're
	;  immutable, we can just return the uncopied object
	%is_string = icmp eq %Object$vtabletype* %vtable, @String$vtable
	%is_int = icmp eq %Object$vtabletype* %vtable, @Int$vtable
	%is_bool = icmp eq %Object$vtabletype* %vtable, @Bool$vtable
	%or_1 = or i1 %is_string, %is_int
	%or_2 = or i1 %or_1, %is_bool
	br i1 %or_2, label %RetThis, label %DoCopy

DoCopy:
	; Allocate object
	%new = call fastcc %Object* @alloc_object(%Object$vtabletype* %vtable)

	; Call copy constructor
	%construct_ptr = getelementptr inbounds %Object$vtabletype, %Object$vtabletype* %vtable, i32 0, i32 4
	%construct = load void (%Object*, %Object*)*, void (%Object*, %Object*)** %construct_ptr
	call fastcc void %construct(%Object* %new, %Object* %this)
	ret %Object* %new

RetThis:
	ret %Object* %this
}

; Returns the name of the type of an object
define hidden fastcc %String* @Object.type_name(%Object* %this)
{
	; Extract type_name from vtable
	%vtable_ptr = getelementptr inbounds %Object, %Object* %this, i32 0, i32 0
	%vtable = load %Object$vtabletype*, %Object$vtabletype** %vtable_ptr

	%type_name_ptr = getelementptr inbounds %Object$vtabletype, %Object$vtabletype* %vtable, i32 0, i32 2
	%type_name = load %String*, %String** %type_name_ptr

	; Increment refcount on string
	%type_name_obj = getelementptr inbounds %String, %String* %type_name, i32 0, i32 0
	call fastcc void @refcount_inc(%Object* %type_name_obj)

	; Return final string
	ret %String* %type_name
}

; Prints a string (returns self)
define hidden fastcc %IO* @IO.out_string(%IO* %this, %String* %value)
{
	; Get string length and data pointer
	%str_len = call fastcc i32 @String.length(%String* %value)
	%str_data = getelementptr inbounds %String, %String* %value, i32 0, i32 2, i32 0

	; Call printf
	%format = getelementptr inbounds [5 x i8], [5 x i8]* @format_str, i32 0, i32 0
	call i32 (i8*, ...) @printf(i8* %format, i32 %str_len, i8* %str_data)

	; Return this
	%this_obj = getelementptr inbounds %IO, %IO* %this, i32 0, i32 0
	call fastcc void @refcount_inc(%Object* %this_obj)
	ret %IO* %this
}

; Prints an integer (returns self)
define hidden fastcc %IO* @IO.out_int(%IO* %this, i32 %value)
{
	; Call printf
	%format = getelementptr inbounds [3 x i8], [3 x i8]* @format_int, i32 0, i32 0
	call i32 (i8*, ...) @printf(i8* %format, i32 %value)

	; Return this
	%this_obj = getelementptr inbounds %IO, %IO* %this, i32 0, i32 0
	call fastcc void @refcount_inc(%Object* %this_obj)
	ret %IO* %this
}

; Reads one line of input (does not read new line character)
define hidden fastcc %String* @IO.in_string(%IO* %this)
{
	%buf = alloca i8, i32 4096

	; Run fgets
	%stdin = load %IO$File*, %IO$File** @stdin
	%fgets_result = call i8* @fgets(i8* %buf, i32 4096, %IO$File* %stdin)

	; Check for EOF
	%is_eof = icmp eq i8* %fgets_result, null
	br i1 %is_eof, label %Eof, label %MakeString

MakeString:
	; Get string size
	%orig_str_len = call i32 @strlen(i8* %buf)

	; Erase \n if it is at the end
	%newline_pos = sub nuw i32 %orig_str_len, 1
	%newline_ptr = getelementptr inbounds i8, i8* %buf, i32 %newline_pos
	%newline_char = load i8, i8* %newline_ptr
	%is_newline = icmp eq i8 %newline_char, 10

	%str_len = select i1 %is_newline, i32 %newline_pos, i32 %orig_str_len

	; Allocate string object
	%str = call fastcc %String* @alloc_string(i32 %str_len)

	; Copy string data
	%str_data_ptr = getelementptr %String, %String* %str, i32 0, i32 2, i32 0
	call void @llvm.memcpy.p0i8.p0i8.i32(i8* %str_data_ptr, i8* %buf, i32 %str_len, i32 0, i1 0)

	ret %String* %str

Eof:
	; Return the empty string
	%empty_object = getelementptr inbounds %String, %String* @String$empty, i32 0, i32 0
	call fastcc void @refcount_inc(%Object* %empty_object)
	ret %String* @String$empty
}

; Reads one line of input and parses it as an integer
define hidden fastcc i32 @IO.in_int(%IO* %this)
{
	%buf = alloca i8, i32 4096

	; Run fgets
	%stdin = load %IO$File*, %IO$File** @stdin
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
define hidden fastcc %Object* @Int$box(i32 %value)
{
	; Allocate integer object
	%new = call fastcc %Object* @alloc_object(%Object$vtabletype* @Int$vtable)

	; Store value into it
	%new_int = bitcast %Object* %new to %Int*
	%value_ptr = getelementptr inbounds %Int, %Int* %new_int, i32 0, i32 1
	store i32 %value, i32* %value_ptr

	ret %Object* %new
}

; Boxes a boolean into an object
define hidden fastcc %Object* @Bool$box(i1 %value)
{
	; Allocate boolean object
	%new = call fastcc %Object* @alloc_object(%Object$vtabletype* @Bool$vtable)

	; Store value into it
	%new_bool = bitcast %Object* %new to %Bool*
	%value_ptr = getelementptr inbounds %Bool, %Bool* %new_bool, i32 0, i32 1
	store i1 %value, i1* %value_ptr

	ret %Object* %new
}

; Unboxes an integer
define hidden fastcc i32 @Int$unbox(%Object* %value) inlinehint
{
	; Load directly from object
	%value_as_int_ptr = bitcast %Object* %value to %Int*
	%value_ptr = getelementptr inbounds %Int, %Int* %value_as_int_ptr, i32 0, i32 1
	%result = load i32, i32* %value_ptr
	ret i32 %result
}

; Unboxes a boolean
define hidden fastcc i1 @Bool$unbox(%Object* %value) inlinehint
{
	; Load directly from object
	%value_as_bool_ptr = bitcast %Object* %value to %Bool*
	%value_ptr = getelementptr inbounds %Bool, %Bool* %value_as_bool_ptr, i32 0, i32 1
	%result = load i1, i1* %value_ptr
	ret i1 %result
}

; Returns the string's length
define hidden fastcc i32 @String.length(%String* %this) inlinehint
{
	; Test for null string
	%this_as_object = getelementptr inbounds %String, %String* %this, i32 0, i32 0
	call fastcc void @null_check(%Object* %this_as_object)

	; Return length
	%length_ptr = getelementptr inbounds %String, %String* %this, i32 0, i32 1
	%length = load i32, i32* %length_ptr
	ret i32 %length
}

; Concatenates two strings
define hidden fastcc %String* @String.concat(%String* %this, %String* %other)
{
	; Test if any inputs are null
	%this_as_object = getelementptr inbounds %String, %String* %this, i32 0, i32 0
	call fastcc void @null_check(%Object* %this_as_object)
	%other_as_object = getelementptr inbounds %String, %String* %other, i32 0, i32 0
	call fastcc void @null_check(%Object* %other_as_object)

	; Get length of new string
	%this_len_ptr = getelementptr inbounds %String, %String* %this, i32 0, i32 1
	%this_len = load i32, i32* %this_len_ptr
	%other_len_ptr = getelementptr inbounds %String, %String* %other, i32 0, i32 1
	%other_len = load i32, i32* %other_len_ptr
	%new_len = add nuw i32 %this_len, %other_len

	; Allocate string
	%new = call fastcc %String* @alloc_string(i32 %new_len)

	; Calculate data pointers
	%new_data_ptr   = getelementptr %String, %String* %new, i32 0, i32 2, i32 0
	%new_data_ptr2  = getelementptr i8, i8* %new_data_ptr, i32 %this_len
	%this_data_ptr  = getelementptr %String, %String* %this, i32 0, i32 2, i32 0
	%other_data_ptr = getelementptr %String, %String* %other, i32 0, i32 2, i32 0

	; Copy data
	call void @llvm.memcpy.p0i8.p0i8.i32(i8* %new_data_ptr,  i8* %this_data_ptr,  i32 %this_len,  i32 0, i1 0)
	call void @llvm.memcpy.p0i8.p0i8.i32(i8* %new_data_ptr2, i8* %other_data_ptr, i32 %other_len, i32 0, i1 0)

	ret %String* %new
}

; Extracts a substring of this string
define hidden fastcc %String* @String.substr(%String* %this, i32 %i, i32 %l)
{
	; Test for null string
	%this_as_object = getelementptr inbounds %String, %String* %this, i32 0, i32 0
	call fastcc void @null_check(%Object* %this_as_object)

	; Get original string length
	%this_len_ptr = getelementptr inbounds %String, %String* %this, i32 0, i32 1
	%this_len = load i32, i32* %this_len_ptr

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
	%new_data_ptr = getelementptr %String, %String* %new, i32 0, i32 2, i32 0
	%old_data_ptr = getelementptr %String, %String* %this, i32 0, i32 2, i32 %i
	call void @llvm.memcpy.p0i8.p0i8.i32(i8* %new_data_ptr, i8* %old_data_ptr, i32 %l, i32 0, i1 0)
	ret %String* %new

Self:
	; Return this with no changes
	%this_object = getelementptr inbounds %String, %String* %this, i32 0, i32 0
	call fastcc void @refcount_inc(%Object* %this_object)
	ret %String* %this

Empty:
	; Return the empty string
	%empty_object = getelementptr inbounds %String, %String* @String$empty, i32 0, i32 0
	call fastcc void @refcount_inc(%Object* %empty_object)
	ret %String* @String$empty

RangeError:
	call fastcc void @abort_with_msg(i8* getelementptr inbounds ([35 x i8], [35 x i8]* @err_range, i32 0, i32 0))
	unreachable
}

define hidden fastcc i1 @String$equals(%String* %a, %String* %b)
{
	; Get lengths
	%a_len_ptr = getelementptr inbounds %String, %String* %a, i32 0, i32 1
	%a_len = load i32, i32* %a_len_ptr
	%b_len_ptr = getelementptr inbounds %String, %String* %b, i32 0, i32 1
	%b_len = load i32, i32* %b_len_ptr

	; Get initial data pointers
	%a_data_start = getelementptr inbounds %String, %String* %a, i32 0, i32 2, i32 0
	%b_data_start = getelementptr inbounds %String, %String* %b, i32 0, i32 2, i32 0

	; Lengths must be identical
	%lengths_equal = icmp eq i32 %a_len, %b_len
	br i1 %lengths_equal, label %FullCompare, label %Fail

FullCompare:
	; Get pointers and counter
	%bytes_left = phi i32 [ %a_len, %0 ], [ %new_len, %FullCompare2 ]
	%a_data = phi i8* [ %a_data_start, %0 ], [ %new_a_data, %FullCompare2 ]
	%b_data = phi i8* [ %b_data_start, %0 ], [ %new_b_data, %FullCompare2 ]

	; Finished?
	%done = icmp eq i32 %bytes_left, 0
	br i1 %done, label %Pass, label %FullCompare2

FullCompare2:
	; Compare next character
	%deref_a = load i8, i8* %a_data
	%deref_b = load i8, i8* %b_data
	%equal = icmp eq i8 %deref_a, %deref_b

	%new_len = sub nuw i32 %bytes_left, 1
	%new_a_data = getelementptr inbounds i8, i8* %a_data, i8 1
	%new_b_data = getelementptr inbounds i8, i8* %b_data, i8 1
	br i1 %equal, label %FullCompare, label %Fail

Pass:
	ret i1 1

Fail:
	ret i1 0
}
