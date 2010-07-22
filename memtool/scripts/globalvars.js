
//var i = new Instance(0x2a48f);
//
//print(i.toString());


// Retrieve list of all global variable names
var varIds = getVariableIds();

for (var i = 0; i < varIds.length; ++i) {
	print(i + ": 0x" + varIds[i].toString(16));
	// Create an instance of each variable
	var inst = new Instance(varIds[i]);
	// Output some information about it
	print("0x" + inst.Id().toString(16) + " " + 
			inst.Name() + ": " + inst.TypeName() + " @ 0x" + inst.Address() + 
			", size = " + inst.Size() + " bytes, real type = " + inst.Type());
}

