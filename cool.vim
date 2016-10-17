" Vim syntax file
" Language:	Cool (Classroom Object Oriented Language)
" Maintainer:	James Cowgill <james410@cowgill.org.uk>
" Last Change:	2016 Oct 16

" quit when a syntax file was already loaded
if exists("b:current_syntax")
  finish
endif

" Keywords
syn keyword coolConditional	case else esac fi if of then
syn keyword coolRepeat		loop pool while
syn keyword coolOperator	isvoid new not '+' '-' '*' '/' '~' '<' '<=' '=' '<-' '=>'
syn keyword coolBoolean		true false
syn keyword coolStructure	class
syn keyword coolKeywords	in inherits let

" Integers
syn match coolInteger		'[0-9][0-9]*'

" Identifiers
syn match coolIdentifier	'[a-z][a-z0-9_]*'
syn match coolType		'[A-Z][a-z0-9_]*'

" Strings
syn match coolStringEscape	contained '\[\bfnrt]'
syn region coolString		start=+"+ end=+"+ contains=coolStringEscape,@Spell

" Comments
syn match coolCommentSingle	'--.*'
syn region coolCommentMulti	start='(\*' end='\*)'

" Highlighting Links
hi def link coolCommentSingle	Comment
hi def link coolCommentMulti	Comment

hi def link coolConditional	Conditional
hi def link coolRepeat		Repeat
hi def link coolOperator	Operator
hi def link coolStructure	Structure
hi def link coolKeywords	Keyword

hi def link coolIdentifier	Identifier
hi def link coolType		Type

hi def link coolBoolean		Boolean
hi def link coolInteger		Integer
hi def link coolString		String

let b:current_syntax = "cool"
