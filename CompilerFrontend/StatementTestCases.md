# Statement Test Case Methodology ##################

A test case methodology for the BASIC statement parser.


## Test Cases ######

### Successful Parses ######

	Beep
	Beep(3)
	System.Beep
	System.Beep()
	System.Beep(3)
	System.UI.Beep
	System.Process(1).Activate
	System.Process(1).Activate(System.Process.Behind, System.Process.Now)
	Console.WriteLine "Hello World!"
	Console.WriteLine("Hello World!")
	self.DogsAge = 1 * inX + 2
	self.Dog(7) = new Dog("Fido", 3)
	self.Dog(3) = inAnimals(16).getDog(14)
	self.Title = inMofset.getName() + " " + inBoggle.getSex()
	x = z5 ((2 - -y) And (bob.theBuilder = "cool")) + 9
	z5 (("cool") + str(5)), pickle
	z5 ("cool"), pickle


### Syntax Errors ######

	x = z5((2 - -y) And (bob.theBuilder = "cool")), picle
	z5 + (2 - -y) = 7 And (bob.theBuilder = "cool")


## Methodology #######

*	All valid permutations

*	Calls
	*	Parameters
		*	Unbracketed
		*	Bracketed
		*	None
		*	One
		*	Two
		*	Three
		*	Simple literals
		*	Complex expressions
			*	Literal
			*	Subexpression
			*	Function call
			*	Negation operator
			*	Logical Not
			*	Equal operator
	*	Super constructor
	*	Super method implementation
	*	This method
	*	Unscoped method
	*	Namespace, class method
	*	Class method
	*	Local array, indexed element, object method

*	Assignments
	*	Target
		*	Local
		*	Scoped instance variable
		*	Scoped object property
		*	Array
	*	Value
		*	Complex expressions (as above)
	
*	Pragma
	*	Always fixed two items; an identifier and a literal


### Error Cases ##########

*	Double operators in expression
*	Double literal / path / subexpression in expression
*	Empty list item
*	Too many closing parentheses
*	Not enough closing parentheses



## Document Control #######

Copyright (c) 2013 Joshua Hawcroft
Last revision 1.0, 17 March 2013


