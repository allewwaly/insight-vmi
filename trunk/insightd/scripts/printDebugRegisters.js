
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

var pid_size = 8;
var register_size = 60;
var cmd_size = 18;
var addr_size = 18;

function printList(p)
{
	if (p.IsNull()) {
		return;
	}
	
	var it = p;
	var debug;
	var debugString = "";
	
	do {
		// Get the registers
		debug = it.thread;
		debug.AddToAddress(parseInt("30", 16));
		debugString = debug.toUInt32().toString(16);
		debug.AddToAddress(parseInt("4", 16));
		debugString += ", " + debug.toUInt32().toString(16);
		debug.AddToAddress(parseInt("4", 16));
		debugString += ", " + debug.toUInt32().toString(16);
		debug.AddToAddress(parseInt("4", 16));
		debugString += ", " + debug.toUInt32().toString(16);
		debug.AddToAddress(parseInt("4", 16));
		debugString += ", " + debug.toUInt32().toString(16);
		debug.AddToAddress(parseInt("4", 16));
		debugString += ", " + debug.toUInt32().toString(16);
		debug.AddToAddress(parseInt("4", 16));
		debugString += ", " + debug.toUInt32().toString(16);
		debug.AddToAddress(parseInt("4", 16));
		debugString += ", " + debug.toUInt32().toString(16);
		
		var line = 
			ralign(it.pid.toString(), pid_size) + 
			"  " +
			lalign(it.comm.toString(), cmd_size) +
			lalign("0x" + it.Address(), addr_size) +
			"\n" +
			lalign("DEBUG REGISTER: " + debugString, register_size);

		print(line);
		
		it = it.tasks.next;
	} while (it.pid.toUInt32() != 0);
}

function printHdr()
{
	var hdr = 
		ralign("PID", pid_size) + 
		"  " + 
		lalign("COMMAND", cmd_size) +
		lalign("ADDRESS", addr_size);
	print(hdr);
	print("===============================================================");
}

var init = new Instance("init_task");

printHdr();
printList(init);

