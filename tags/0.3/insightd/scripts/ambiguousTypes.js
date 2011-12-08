
include("lib_string.js");

var w_name = 35;
var w_type = 15;
var w_size = 15;
var w_total = w_name + w_type + w_size;

print("Checking for types with the same name but different, non-null sizes.");
print();
print(
	lalign("Type name", w_name) +
	lalign("Base type", w_type) +
	ralign("Sizes", w_size)
);
print(hline(w_total));

// Get all type names
var typeNames = Symbols.typeNames();
var count = 0;

for (var i in typeNames) {
	// Find all types with that name
	var types = Symbols.listTypes(typeNames[i]);
	var sizes = new Array;
	
	for (var t in types) {
		// Skip types with zero length
		if (!types[t].Size())
			continue;
		// Check if we already have that size
		var found = false;
		for (var s in sizes) {
			if (sizes[s] == types[t].Size()) {
				found = true;
				break;
			}
		}
		if (!found)
			sizes.push(types[t].Size());
	}
	
	if (sizes.length > 1) {
		sizes.sort();
		print(
			lalign(types[0].TypeName(), w_name) +
			lalign(types[0].Type(), w_type) +
			ralign(sizes.join(", "), w_size)
		);
		count++;
	}
}

print(hline(w_total));
print("Total: " + count + " ambiguous types");