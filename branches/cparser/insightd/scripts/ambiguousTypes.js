
include("lib_string.js");

var w_name = 40;
var w_type = 15;
var w_size = 15;
var w_total = w_name + w_type + w_size + 2;

println("Checking for types with the same name but different, non-null sizes.");
println();
println(
	lalign("Type name", w_name) + " " +
	lalign("Base type", w_type) + " " +
	ralign("Sizes", w_size)
);
println(hline(w_total));

// Get all type names
var typeNames = Symbols.typeNames();
var count = 0;
var errors = new Array;

for (var i in typeNames) {
	// Find all types with that name
	var types = Symbols.listTypes(typeNames[i]);
	var sizes = new Array;

    for (var t in types) {
        try {
            // Skip types with zero length
            if (!types[t].Size())
                continue;
            // Skip functions
            if (types[t].Type() == "Function")
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
        catch (e) {
            errors.push("### Type 0x" + types[t].TypeId().toString(16) + ": " + e);
            continue;
        }
    }

    if (sizes.length > 1) {
        try {
            sizes.sort();
            println(
                lalign(types[0].TypeName(), w_name) + " " +
                lalign(types[0].Type(), w_type) + " " +
                ralign(sizes.join(", "), w_size)
            );
            count++;
        }
        catch (e) {
            errors.push("### Type 0x" + types[t].TypeId().toString(16) + ": " + e);
            continue;
        }
    }
}

println(hline(w_total));
println("Total: " + count + " ambiguous types");

if (errors.length > 0) {
    println();
    println("A total of", errors.length, "errors occured:");
    for (var i in errors)
        println(errors[i]);
}

