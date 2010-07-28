
//Prints a string with a given width left-aligned
function lalign(s, len)
{
	while (len > 0 && s.length < len)
		s += " ";
	return s;
}

//Prints a string with a given width right-aligned
function ralign(s, len)
{
	while (len > 0 && s.length < len)
		s = " " + s;
	return s;
}

function doCompare(inst1, inst2)
{
    if (inst1.Equals(inst2))
        print("Equal:     [" + inst1.MemDumpIndex() + "] " + inst1.FullName() +
              ", [" + inst2.MemDumpIndex() + "] " + inst2.FullName());
    else
        print("Different: [" + inst1.MemDumpIndex() + "] " + inst1.FullName() +
              ", [" + inst2.MemDumpIndex() + "] " + inst2.FullName());
}


function showDifferences(inst1, inst2, recursive)
{
	var diff = inst1.Differences(inst2, recursive);
//    for (i in diff)
//        print("diff[" + i + "] = \"" + diff[i] + "\"");

	var size_name = 0, size_val = 0;
	for (i in diff) {
		var n = diff[i];
		if (n.length <= 0)
			continue;
		if (n.length > size_name)
			size_name = n.length;
		if (inst1[n].toString().length > size_val)
			size_val = inst1[n].toString().length;
		if (inst2[n].toString().length > size_val)
			size_val = inst2[n].toString().length;
	}
	
	
    print("Differences for [" + inst1.MemDumpIndex() + "] " + inst1.FullName() +
            ", [" + inst2.MemDumpIndex() + "] " + inst2.FullName() + ":");
    if (diff.length <= 0)
    	print("  (instances are equal)");
    else if (diff[0].length == 0)
    	print("  (instances are uncomparable)");
    else {
    	for (i in diff) {
    		var n = diff[i];
    		var val1 = inst1[n].IsNumber() ? 
    				ralign(inst1[n].toString(), size_val) :
    				lalign(inst1[n].toString(), size_val);
    		var val2 = inst2[n].IsNumber() ? 
    				ralign(inst2[n].toString(), size_val) :
    				lalign(inst2[n].toString(), size_val);
	        print("  " + lalign(n + ": ", size_name + 2) + val1 + ", " + val2);
    	}
    }
}


var dumps = getMemDumps();

if (dumps.length < 2) {
	print("We need at least two loaded memory dumps for this script.");
}
else {
	print("Comparing some instances between the following memory dumps:");
	print("[0] " + dumps[0]);
	print("[1] " + dumps[1]);
    print();
	
    var inst1 = new Instance("init_task", 0);
    var inst2 = new Instance("init_task", 1);

    doCompare(inst1, inst1);
    doCompare(inst2, inst2);
    doCompare(inst1, inst2);

    doCompare(inst1.pid, inst2.pid);
    doCompare(inst1.comm, inst2.comm);
    doCompare(inst1.tasks, inst2.tasks);
    doCompare(inst1.timestamp, inst2.timestamp);

    doCompare(inst1, inst1.children.next);
    doCompare(inst1.user.uid, inst1.tasks.next.user.uid);
    doCompare(inst1.comm, inst1.tasks.next.comm);
    doCompare(inst1.comm, inst2.pid);
    
    print();

    print("Detailed comparison of two struct instances:");
    print();
    showDifferences(inst1, inst1, false);
    print();
    showDifferences(inst1, inst2, false);
    print();
    showDifferences(inst1, inst1.tasks.next, false);

    print();
    
    print("Recursive comparison of two struct instances:");
    print();
    showDifferences(inst1.user, inst2.user, true);
    print();
    showDifferences(inst1, inst2.user, true);
    print();
    showDifferences(new Instance("init_mm", 0), new Instance("init_mm", 1), true);
    print();
    try {
	    showDifferences(inst1, inst2, true);
    }
    catch(err) {
    	print(err);
    }
    print();
}



