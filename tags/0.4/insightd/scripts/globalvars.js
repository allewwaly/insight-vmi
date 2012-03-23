
include("lib_string.js");

// Retrieve list of all global variable IDs
var varIds = Symbols.variableIds();
varIds = varIds.sort();

// What is the max. ID?
var maxId = varIds.length > 0 ? varIds[varIds.length - 1] : 0;

var size_name = 35;
var size_typeName = 25;
var size_type = 7;
var size_val = 20;
var size_id = 1;
while (maxId > 0) {
	maxId >>= 4;
	size_id++;
}

// Iterate over all variable IDs
for (var i = 0; i < varIds.length; ++i) {
//	print(i + ": 0x" + varIds[i].toString(16));
	// Create an instance of each variable
	var inst = new Instance(varIds[i]);
	// Collect some information about it
	var name = lalign(inst.Name(), size_name);
	var typeName = name.length > size_name ? 
			inst.TypeName() : 
			lalign(inst.TypeName(), size_typeName)
	var val;
	try {
		if (inst.Type() == "Struct" || inst.Type() == "Union")
			val = "...";
		else {
			val = inst.toString();
			if (val.length > size_val)
				val = val.substr(0, size_val - 3) + "...";
		}
	}
	catch (err) {
		val = "(not accessible)"
	}
			
	// Output the information
	print(ralign("0x" + inst.Id().toString(16), size_id) + " " + 
		  lalign(name + ": " + typeName,
				 size_name + size_typeName + 2) + 
//		  " " + 
//		  lalign("(" + inst.Type() + ")", size_type) +
		  " @ 0x" + inst.Address() +
		  " = " +
		  lalign(val, size_val));

//    try {
//		print(inst.toString());
//    }
//    catch (err) {
//    	print(err);
//    }
			
}

print("Total number of global variables: " + varIds.length)

