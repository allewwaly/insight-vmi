// Check the argument(s)
if (ARGV.length == 1) {
	print("Usage: " + ARGV[0] + " <var_name> [<var_name> ...]");
	print("Just try \"*\" as variable name.");
}
else {
	include("lib_string.js");

	for (var i = 0; i < ARGV.length; ++i) {
		var list = Symbols.listVariables(ARGV[i]);
		for (var j in list) {
			print(
				ralign(uhex(list[j].TypeId()), 8) + " " +
				lalign(
					lalign(list[j].Name() + ": ", 25) +
					list[j].TypeName(), 65) +
				" @ 0x" + list[j].Address().toString(16));
		}
	}
}
