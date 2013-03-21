Event Model for Runless Basic
=============================

Objectives
----------

Eventually I would like to achieve two things:

*	An IDE with a similar usability and intuitiveness to that of [REALstudio] or [VisualBasic].

*	A language powerful enough to enable runtime, programatic window creation and modification.

The event handling model of the language is influenced by these objectives.

[REALstudio]: http://www.realsoftware.com
[VisualBasic]: http://www.microsoft.com


Proposed Implementation
-----------------------

*	Declaration of events that a class supports, using the Event keyword:
	
		Event Click
	
	With a syntax that parallels functions and subroutines for argument and return type declaration.
	
*	Declaration of handlers for a super class's events using the Handler keyword:
	
		Handler Click
	
	Again, syntax parallels that of a function or subroutine.

*	Windows allow controls to be added programatically and the IDE uses this mechanism behind the scenes to implement visual configuration of windows.
	
	The method that adds controls to a window will require a Name (String) as an argument.

*	The event handlers for controls added to a window are of the form:

		Handler ControlName.Click
	
	Where ControlName is the name assigned the control when it was added to the window, and Click is the event to be handled.
	
	These appear in the window class' source file.
