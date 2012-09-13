
var dumps = Memory.list();

if (dumps.length <= 0)
	print("No memory dumps loaded.");
else {
    for (var i  in dumps)
        println("[" + i + "] " + dumps[i]);
}
