
include("lib_string.js");

var w_id = 8;
var w_name = 40;
var w_member = 20;
var w_alt = 4;
var w_total = w_id + w_name + w_member + w_alt + 3;

var minAlt = (ARGV.length > 1) ? ARGV[1] : 1;


print("Searching struct members with at least " + minAlt.toString() + " candidates.");
print();
print(
	lalign("ID", w_id) + " " +
	lalign("Type name", w_name) + " " +
	lalign("Member", w_member) + " " +
	ralign("Alt.", w_alt)
);
print(hline(w_total));

// Get all type names
var types = Symbols.listTypes();
var tcount = 0;
var mcount = 0;
var lastType = 0;
var typeNames = new Array();
var typeNameCount = 0;

for (var i in types) {
	// Get instance of that type
	var type = types[i];
	// We are only looking for structs with members
	if (type.MemberCount() <= 0)
		continue;

	for (var j = 0; j < type.MemberCount(); ++j) {
		// Skip members without candidates
		if (type.MemberCandidatesCount(j) < minAlt)
			continue;

		// Count types with candidates
		if (lastType != type.TypeId()) {
			lastType = type.TypeId();
			++tcount;
		}
		// Count unique types by hash
		if (typeNames.indexOf(type.TypeHash()) < 0) {
			++typeNameCount;
			typeNames.push(type.TypeHash());
		}

		print(
			lalign(uhex(type.TypeId()), w_id) + " " +
			lalign(type.TypeName(), w_name) + " " +
			lalign(type.MemberNames()[j], w_member) + " " +
			ralign(type.MemberCandidatesCount(j), w_alt)
		);
		++mcount
	}
}

print(hline(w_total));
print("Total: " + mcount + " members in " + tcount + " types, " + typeNameCount + " unique types");
