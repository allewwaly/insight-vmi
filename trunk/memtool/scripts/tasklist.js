
print("===================================================");

function dumpInstance(inst)
{
	print(inst.name() + " = \"" + inst.typeName() + "\" @ 0x" + inst.address().toString(16));
	print(inst.toString());
	print("-------------------------------------------------------");
}

function lalign(s, len)
{
	while (len > 0 && s.length < len)
		s += " ";
	return s;
}

function ralign(s, len)
{
	while (len > 0 && s.length < len)
		s = " " + s;
	return s;
}

var uid_size = 8;
var pid_size = 8;
var gid_size = 8;
var cmd_size = 18;

function printSiblings(p)
{
//	if (!(p instanceof Instance)) {
//		print("This is not an Instance object!, p = " + p);
//		return;
//	}
//	print("+++ Entering printSiblings()");
	
	if (p.isNull()) {
//		print("+++ Returning from printSiblings()");
		return;
	}
	
	var it = p;
//	print("p.parent.address() = 0x" + p.parent.address().toString(16));
	
	do {
		var line = 
			lalign(it.uid.toString(), uid_size) + 
			ralign(it.pid.toString(), pid_size) + 
			ralign(it.gid.toString(), gid_size) +
			"  " +
			lalign(it.comm.toString(), cmd_size) +
			"  @ 0x" + it.address().toString(16);

		print(line);
		
		printSiblings(it.children.next);
		
		it = it.sibling.next;
//		print ("it.address() = 0x" + it.address().toString(16) + ", ");
	} while (it.address() != p.parent.address() && it.address() != p.address());


//	print("+++ Leaving printSiblings()");
}

var hdr = 
	lalign("USER", uid_size) + 
	ralign("PID", pid_size) + 
	ralign("GID", gid_size) +
	"  " + 
	lalign("COMMAND", cmd_size);

print(hdr);

var it = new Instance("init_task");

//if (!(it instanceof Instance))
//	print("Not an Instance object: " + it);
printSiblings(it.children.next, it);

//dumpThis(it.pid);


//print("it = \"" + it.typeName() + "\" @ 0x" + it.address().toString(16));
//print(it.pid.toString());
//dumpInstance(it.pid);

//ch = it.children;
//dumpInstance(ch);

//dumpInstance(ch.next.pid);

//print("------------");


//var names = ch.memberNames();
//print("names = " + names);
//
//for (var n in names) {
//	print(n);
//} 

// for (var i = 0; i < names.length; i++) {
//	print(names[i]);
//}


//var m = ch.members();

//print("The members of " + ch.fullName() + " are:");
//for (m in ch) {
//	print("  name: " + ch[m].fullName() + ", value: " + "...");
//}
//
//print("----------");
//
//print(ch[m].pid.fullName());

//print("children.next = " + ch["next"]);

//
//for (prop in m)
//	print ("it = " + it);
//
//var a = new Array(1, 2, 3);
//print("a = " + typeof a);
//if (a instanceof Array)
//	print("a is an Array");
