
var dumps = getMemDumps();

if (dumps.length <= 0) {
	print("No memory dumps loaded.");
}
else {
	for (var i = 0; i < dumps.length; i++)
	print("[" + i + "] " + dumps[i]);
}
