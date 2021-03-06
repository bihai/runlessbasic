Runless BASIC
=============

Copyright (c) 2013 Joshua Hawcroft <dev@joshhawcroft.com>, <blog.joshhawcroft.com>


About
-----

This is a free software project to develop a BASIC programming environment with a graphical IDE.

Unlike similar projects, Runless will produce native executables _without_ it's own runtime library, or with minimal such dependencies.

The compiler will be written to use a macro system to produce code that links against native APIs directly whenever possible.


Motivation
----------

*	Personally I enjoy compiler development.
*	I like the BASIC language.
*	There is no good, easy-to-use alternative to Cocoa development on Mac.

	Yes, I know about [REALstudio], [LiveCode], [Gambas], [PureBASIC] and the like, but each of these have significant flaws in my opinion.
	
[REALstudio]: http://www.realsoftware.com/
[LiveCode]: http://www.runrev.com/
[Gambas]: http://gambas.sourceforge.net/
[PureBASIC]: http://www.purebasic.com/


License
-------

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
