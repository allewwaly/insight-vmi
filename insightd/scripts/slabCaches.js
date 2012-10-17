
include("lib_string.js");

var name_size = 30;
var addr_size = 18;

function printList(p)
{
	if (p.IsNull()) {
		return;
	}
	
	var it = p;
	var first = it.Address();
	
	do {
		var line = 
			lalign(it.name.toString(), name_size) + 
			lalign("0x" + it.Address(), addr_size);

        println(line);
		
		it = it.next.next;
	} while (it.Address() != first);
}

function printHdr()
{
	var hdr = 
		lalign("Name", name_size) +
		lalign("ADDRESS", addr_size);
	
    println(hdr);
    println("===================================================");
}

var cacheList = new Instance("cache_chain.next");
// Convert type
cacheList.ChangeType(parseInt("40692d", 16));
// Update address
cacheList.AddToAddress(-1 * parseInt("6c", 16));

printHdr();
printList(cacheList);
