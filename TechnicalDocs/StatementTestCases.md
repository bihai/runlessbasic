# Statement Test Case Methodology ##################

A test case methodology for the BASIC statement parser.


## Test Cases ######

### Successful Parses ######

#### Complex Expressions #####

	"The answer is " + Str(42 + (0 * anotherLocal)) + ("." + (" ")) + Str(Not (bob.type = "builder"))
	Format(-x + (y - -z), "-0.00") + " " + pancakes(0).GetTitle("alpha", "beta", petunias.getCount())
	
##### Errors ######
	
	"The answer is " + + Str(42 + (0 * anotherLocal)) + ("." + (" ")) + Str(Not (bob.type = "builder"))
	"The answer is " + Str(42 + (0 * anotherLocal errorLocal)) + ("." + (" ")) + Str(Not (bob.type = "builder"))
	"The answer is " + Str(42 + (0 * anotherLocal)) + ("." + (" ")) + Str(Not (bob.type = "builder")) "Error"
	"The answer is " + Str(42 + (0 * anotherLocal)) + ("." + (" ")("Error")) + Str(Not (bob.type = "builder"))
	Format(-x + (y - -z), "-0.00", ) + " " + pancakes(0).GetTitle("alpha", "beta", petunias.getCount())
	Format(-x + (y - -z), "-0.00") + " " + pancakes(0).GetTitle("alpha", "beta", petunias.getCount()))
	Format(-x + (y - -z), "-0.00") + " " + (pancakes(0).GetTitle("alpha", "beta", petunias.getCount())


#### Assignments #####

Where <complex-expression> is one of the above (Complex Expressions).

	local = <complex-expression>
	self.property = <complex-expression>
	localObjectRef.property = <complex-expression>
	localObjectRef.propertyA.propertyB = <complex-expression>
	localArray(1) = <complex-expression>
	localArray(1,2) = <complex-expression>
	localArray(<complex-expression>) = "Test"
	localArray(<complex-expression-1>, <complex-expression-2>) = "Test"
	localArray(self.property) = <complex-expression>
	localArray(self.propertyA, localB.propertyC) = <complex-expression>


#### Calls #####

	Super.MethodA
	Super.MethodA(1, 2)
	Super.MethodA 1, 2
	Super.MethodA <complex-expression>
	Super.MethodA <complex-expression-1>, <complex-expression-2>
	Super.MethodA(<complex-expression-1>, <complex-expression-2>)
	Self.MethodA
	MethodA
	Namespace1.Class1.MethodA
	Class1.MethodA
	LocalArray(<complex-expression>).Method 42
	LocalArray(<complex-expression>).Method(42)
	LocalArray(<complex-expression>).Method(40, 2)
	LocalArray(<complex-expression>).Method <complex-expression-1>, <complex-expression-2>, 83






#### Original Tests #####

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

##### Errors #####

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
			*	Local
			*	Subexpression
			*	Function call
			*	Negation operator
			*	Logical Not
			*	Equal operator
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
		*	Property of a scoped object property (sub-property)
		*	Array
			*	Simple integer index
			*	Multiple integer indicies
			*	Local index
			*	Scoped object property index
			*	Property of a bi-dimension array of a bi-dimension array
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


