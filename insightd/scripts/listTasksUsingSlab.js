
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

function getTaskStructCache(cache)
{
	if (cache.IsNull()) {
		return;
	}
	
	var it = cache;
	
	while (it.name.toString() != "\"task_struct\"") {
		it = it.next.next;
	} 
	
	return it;
}

var uid_size = 4;
var pid_size = 8;
var gid_size = 8;
var state_size = 8;
var cmd_size = 18;
var addr_size = 18;


function printLine(task)
{
	var line = 
			ralign(task.uid.toString(), uid_size) + 
			ralign(task.pid.toString(), pid_size) + 
			ralign(task.gid.toString(), gid_size) +
			ralign(task.state.toString(), state_size) +
			"  " +
			lalign(task.comm.toString(), cmd_size) +
			lalign("0x" + task.Address(), addr_size);

        println(line);
}

function printList(list, address, boolFree)
{
	if (list.IsNull()) {
		return;
	}
	
	var it = list;
	var task = new Instance("init_task");
	
	while(it.Address() != address)
	{
		// No more free objects in this list.
		if(it.free.toString() == "3489586704")
			break;
		
		/*
        println(it.Address());
        println(it.free.toString());
        println(it.s_mem.toString());
		*/
	
		// Print only empty items
		if(it.free.toString() == it.nodeid.toString() || boolFree)
		{
			task.SetAddress(it.s_mem.toString());
			printLine(task);
		}
		
		it = it.list.next;
	} 
}

function printData(list)
{
	if (list.IsNull()) {
		return;
	}
	
	// Get the partial and the free list
	var partial = list.slabs_partial.next;
	var free = list.slabs_free.next;
	free.AddToAddress(16);
	var full = list.slabs_full.next;
	full.AddToAddress(8);
	
	// Update types to "slab"
	partial.ChangeType(parseInt("40d55f", 16));
	free.ChangeType(parseInt("40d55f", 16));
	full.ChangeType(parseInt("40d55f", 16));
	
	// Print items
	printList(partial, list.Address(), true);
	printList(free, (parseInt(list.Address(), 16) + 16).toString(16), true);
	printList(full, (parseInt(list.Address(), 16) + 8).toString(16), true);
}

function printHdr()
{
	var hdr = 
		ralign("USER", uid_size) + 
		ralign("PID", pid_size) + 
		ralign("GID", gid_size) +
		ralign("STATE", state_size) +
		"  " + 
		lalign("COMMAND", cmd_size) +
		lalign("ADDRESS", addr_size);
	
    println(hdr);
    println("========================================================");
}

var cacheList = new Instance("cache_chain.next");
// Convert type
cacheList.ChangeType(parseInt("40692d", 16));
// Update address
cacheList.AddToAddress(-1 * parseInt("6c", 16));

// Get task_struct cache
var tsCache = getTaskStructCache(cacheList);

// Get the slab lists
var lists = tsCache;
// We need to get the value of the "nodelists" pointer,
// since the tool uses this address as a pointer to a pointer, 
// which is wrong.
// This is hacky but it works :).
lists.AddToAddress(parseInt("30", 16));
var addr = lists.toUInt32();
lists.SetAddress(addr.toString(16));
lists.ChangeType(parseInt("40d6aa", 16));

// Iterate over the partial and the free list
// and print the addresses.
printHdr();
printData(lists);


