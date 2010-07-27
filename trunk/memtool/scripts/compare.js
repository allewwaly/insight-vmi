
var dumps = getMemDumps();


function doCompare(inst1, inst2)
{
    if (inst1.Equals(inst2))
        print("Equal:     [" + inst1.MemDumpIndex() + "] " + inst1.FullName() +
              ", [" + inst2.MemDumpIndex() + "] " + inst2.FullName());
    else
        print("Different: [" + inst1.MemDumpIndex() + "] " + inst1.FullName() +
              ", [" + inst2.MemDumpIndex() + "] " + inst2.FullName());
}



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
}



