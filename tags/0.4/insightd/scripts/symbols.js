
function usage()
{
	print("Usage: " + ARGV[0] + " v|t [<filter>]");
}


function queryTypes(filter)
{
	print("Retrieving types for: " + filter);
	var ret = Symbols.listTypes(filter);

	for (var i in ret) {
		print(i + ". " + ret[i].TypeName() + 
				", size=" + ret[i].Size() +
				", id=0x" + ret[i].TypeId().toString(16));
	}

	print("Total types: " + ret.length);
}

function queryVars(filter)
{
	print("Retrieving variables for: " + filter);
	var ret = Symbols.listVariables(filter);

	for (var i in ret) {
		print(i + ". " + ret[i].Name() + " : " + ret[i].TypeName() +
				" @ 0x" + ret[i].Address() +
				", size=" + ret[i].Size() + 
				", id=0x" + ret[i].Id().toString(16));
	}

	print("Total variables: " + ret.length);
}

var query = "";
if (ARGV.length > 2)
	query = ARGV[2];


if (ARGV.length < 2) {
	usage();
}
else if (ARGV[1] == "v") {
	queryVars(query);
}
else if (ARGV[1] == "t") {
	queryTypes(query);
}
else {
	usage();
}
