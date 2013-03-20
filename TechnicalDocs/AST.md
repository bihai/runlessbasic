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
