Database Requirements Analysis
==============================

Copyright (c) 2013 Joshua Hawcroft


Operation: Search
-----------------

### Input

One or more keywords from the user, or whole fragments.

Examples:

1.	MyFunction
2.	")
	

### Output

Zero or more found offsets across multiple files.

Examples:

1.	MyClass.bas: 234
2.	Foo.bas: 1230


### Database Requirements

*	file
	*	full-text index of source
	*	verbatim copy of the source


Operation: Compile
------------------

### Input

List of source files.
Build configuration.
Compiler database.
Built-ins database.
Extensions database?


### Output

Build artifacts.


### Database Requirements

*	file
	*	verbatim copy of the source

*	class
	*	name
	*	file reference
	*	properties
		*	name
		*	access
		*	class/instance
		*	type
			*	package
			*	class/type
			*	is array?
	*	methods
		*	name
		*	access
		*	class/instance
		*	arguments
			*	name
			*	type
				*	package
				*	class/type
			*	is array?
			*	by reference?
		*	return type
			*	package
			*	class/type
			*	is array?


SQLite
------

SQLite + FTS4 is looking like the ideal choice at this stage for the database components of the compiler.

It will be wrapped in an appropriate API.


Built-ins Database
------------------

What in other languages would constitute the 'runtime library' will in RunlessBASIC be a documented API and translation rules for each platform.

This document proposes that those rules and the API be documented (from the Compiler's perspective) by a database similar to the compiler database produced for each project.

As with the compiler database, the built-ins database would document classes.


It might even be that the database is produced in the same way, by reading a bunch of source files, and that a special syntax be available within the language (probably relying upon #pragma, #if, etc.) to enable complete description of the translation rules within the language itself.

This needs to be developed further.

Can prototype during development of the compiler.




