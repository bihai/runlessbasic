Abstract Syntax Tree (AST)
==========================

The abstract syntax tree is made up of a heirarchy of nodes.


Discussion of Current Issues
----------------------------

At the moment the AST types are not specific to particular language constructs.  I'm dubious about the current structure and suspect it may best be made more construct specific.

For example, AST_LIST is used to represent many things, depending on context.  At the top level of the AST it is a container for class definitions.  While within a method definition it is a container for statements and control structures.

The list vs value type seems to serve the purpose that may have been driving me toward this minimalist structure.  The minimalist structure may make it harder to write the compiler.  It definately makes it harder to humanly understand the output.

Also, operators are currently encoded as strings.  This shouldn't be the case.  String comparison is inefficient compared with members of an enumeration.  An enumeration constructed with appropriate macros ought to be as friendly to human inspection in the text output as the string version, but still employ enumerations beneath.

These issues could be rectified when a working prototype of the system, translating to Objective-C or C and running against either Cocoa or GTK+ is operational.


Nodes
-----

Each node has a type:

*	AST_STATEMENT
*	AST_PATH
*	AST_LIST
*	AST_EXPRESSION
*	AST_CONTROL

*	AST_NULL
*	AST_STRING
*	AST_OPERATOR
*	AST_INTEGER
*	AST_REAL
*	AST_COLOUR
*	AST_BOOLEAN

Types AST_STATEMENT through AST_CONTROL are _list_ types.  The remainder are _value_ types.

List type nodes can reference zero or more child nodes.


Parse Output
------------

The root of the AST produced by a successful parse is a single AST_LIST node.

The children of this node will only represent _class_ definitions.

Breifly, the form of a syntax tree containing one instance of every major element of a working program is as follows:

*	AST_LIST - the root
	*	AST_CONTROL - a class
		*	AST_STRING - "class"
		*	AST_STRING - class name, eg. "CSimple"
		*	AST_PATH - (optional) the super class, eg. "Lang"."Object"
		*	AST_LIST - (optional) list of interfaces implemented
			*	AST_PATH - an interface, eg. "Lang"."Enumerable"
		*	AST_CONTROL - (0+) a method
			*	AST_STRING - "subroutine" or "function"
			*	AST_STRING - subroutine name, eg. "main"
			*	AST_STRING - access: "public", "protected" or "private"
			*	AST_STRING - shared? "instance" or "class"
			*	AST_LIST - (optional) arguments
				*	AST_LIST - an argument
					*	AST_STRING - argument name, eg. "inMessage"
					*	AST_NULL or AST_STRING "()" - normal or array
					*	AST_PATH - type
			*	AST_PATH - (if function) return type
			*	AST_LIST - the method body
				*	zero or more AST_STATEMENT or AST_CONTROL
		*	AST_CONTROL - (0+) a property
			*	AST_STRING - "property"
			*	AST_STRING - property name, eg. "pHouses"
			*	AST_STRING - access: "public", "protected" or "private"
			*	AST_STRING - shared? "instance" or "class"
			*	AST_PATH - type


Similarly, the various control structures can be represented as follows:

*	AST_CONTROL	- IF
	*	AST_STRING - "if"
	*	AST_EXPRESSION - condition 1
	*	AST_LIST - code block 1
	*	...
	*	AST_EXPRESSION - condition N
	*	AST_LIST - code block N
	*	AST_LIST - (optional) ELSE code block
*	AST_CONTROL - SELECT CASE
	*	AST_STRING - "select"
	*	AST_EXPRESSION - base expression
	*	AST_EXPRESSION - case expression 1
	*	AST_LIST - case block 1
	*	...
	*	AST_EXPRESSION - case expression N
	*	AST_LIST - case block N
	*	AST_LIST - (optional) CASE ELSE code block
*	AST_CONTROL - FOR
	*	AST_STRING - "for"
	*	AST_STRING - counter local variable name
	*	AST_EXPRESSION - initial counter value
	*	AST_STRING - "increment" or "decrement"
	*	AST_EXPRESSION - counter limit value
	*	AST_EXPRESSION - (optional) counter step value
	*	AST_LIST - code block
*	AST_CONTROL - FOR EACH
	*	AST_STRING - "foreach"
	*	AST_STRING - local item variable name
	*	AST_EXPRESSION - iterable expression/array expression
	*	AST_LIST - code block
*	AST_CONTROL - WHILE
	*	AST_STRING - "while"
	*	AST_EXPRESSION - condition
	*	AST_LIST - code block
*	AST_CONTROL - DO
	*	AST_STRING - "do"
	*	AST_EXPRESSION - (optional) pre-condition Until
	*	AST_LIST - code block
	*	AST_EXPRESSION - (optional) post-condition Until


Statements have the following forms:

*	AST_STATEMENT - DIM (array)
	*	AST_CONTROL
		*	AST_STRING - "dim"
		*	AST_STRING - local variable name
		*	AST_LIST - array dimensions
			*	AST_EXPRESSION - dimension 1
			*	...
			*	AST_EXPRESSION - dimension N
		*	AST_PATH - type
*	AST_STATEMENT - DIM (not array)
	*	AST_CONTROL
		*	AST_STRING - "dim"
		*	AST_LIST - variable name(s)
			*	AST_STRING - variable name 1
			*	...
			*	AST_STRING - variable name N
		*	AST_OPERATOR: "new" (optional)
		*	AST_PATH - type
		*	AST_EXPRESSION - (optional) initaliser
*	AST_STATEMENT - break from select or loop
	*	AST_STRING - "break"
*	AST_STATEMENT - continue next iteration of loop
	*	AST_STRING - "continue"
*	AST_STATEMENT - compiler #pragma
	*	AST_STRING - "pragma"
	*	AST_STRING - pragma name, eg. "BoundsChecks"
	*	pragma value: AST_STRING, AST_INTEGER, or AST_REAL
*	AST_STATEMENT - assignment
	*	AST_PATH - destination
	*	AST_EXPRESSION - source
*	AST_STATEMENT - method
	*	AST_PATH - method call


Finally, AST_PATH type nodes take the following form: AST_STRING \[AST_LIST\] ...
where AST_LIST is a list of arguments or array indicies.

AST_EXPRESSION nodes will only ever contain a combination of AST_OPERATOR, AST_EXPRESSION and any of the value node types: AST_NULL, AST_STRING, etc.





