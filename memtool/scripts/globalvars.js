
// Retrieve list of all global variable names
var varNames = listVariables();

for (var i = 0; i < varNames.length; ++i) {
	// Create an instance of each variable
//	var inst = new Instance(varNames[i]);
	print(i + ": " + varNames[i]);
	// Output some information about it
//	if (inst.addressLow() == 0)
//		print("### Error creating instance for \"" + varNames[i] + "\"");
//	else	
//		print(varNames[i] + ": " /*inst.typeName() +*/ 
//				/*", size = " + inst.size() +*/ /*" bytes, real type = " + inst.type()*/ );
}