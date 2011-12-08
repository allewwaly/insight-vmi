
var dumps = getMemDumps();

if (dumps.length <= 0) {
	print("No memory dumps loaded.");
}
else {
    for (var i  in dumps)
        print("[" + i + "] " + dumps[i]);
}
