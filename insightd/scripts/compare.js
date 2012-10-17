
include("lib_string.js");

function doCompare(inst1, inst2)
{
    if (inst1.Equals(inst2))
        println("Equal:     [" + inst1.MemDumpIndex() + "] " + inst1.FullName() +
              ", [" + inst2.MemDumpIndex() + "] " + inst2.FullName());
    else
        println("Different: [" + inst1.MemDumpIndex() + "] " + inst1.FullName() +
              ", [" + inst2.MemDumpIndex() + "] " + inst2.FullName());
}


function showDifferences(inst1, inst2, recursive)
{
	var diff = inst1.Differences(inst2, recursive);
//    for (i in diff)
//        println("diff[" + i + "] = \"" + diff[i] + "\"");

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
	
	
    println("Differences for [" + inst1.MemDumpIndex() + "] " + inst1.FullName() +
            ", [" + inst2.MemDumpIndex() + "] " + inst2.FullName() + ":");
    if (diff.length <= 0)
        println("  (instances are equal)");
    else if (diff[0].length == 0)
        println("  (instances are uncomparable)");
    else {
    	for (i in diff) {
    		var n = diff[i];
    		var val1 = inst1[n].IsNumber() ? 
    				ralign(inst1[n].toString(), size_val) :
    				lalign(inst1[n].toString(), size_val);
    		var val2 = inst2[n].IsNumber() ? 
    				ralign(inst2[n].toString(), size_val) :
    				lalign(inst2[n].toString(), size_val);
            println("  " + lalign(n + ": ", size_name + 2) + val1 + ", " + val2);
    	}
    }
}


var dumps = Memory.list();

if (dumps.length < 2) {
    println("We need at least two loaded memory dumps for this script.");
}
else {
    println("Comparing some instances between the following memory dumps:");
    println("[0] " + dumps[0]);
    println("[1] " + dumps[1]);
    println();
	
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
    
    println();

    println("Detailed comparison of two struct instances:");
    println();
    showDifferences(inst1, inst1, false);
    println();
    showDifferences(inst1, inst2, false);
    println();
    showDifferences(inst1, inst1.tasks.next, false);

    println();
    
    println("Recursive comparison of two struct instances:");
    println();
    showDifferences(inst1.user, inst2.user, true);
    println();
    showDifferences(inst1, inst2.user, true);
    println();
    showDifferences(new Instance("init_mm", 0), new Instance("init_mm", 1), true);
    println();
    try {
	    showDifferences(inst1, inst2, true);
    }
    catch(err) {
        println(err);
    }
    println();
}



