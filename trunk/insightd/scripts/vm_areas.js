
include("lib_string.js");
include("lib_util.js");


var c_addr = 2+16;
var c_size = 8;
var c_io = 5;
var c_type = 10;

var flags_list = [
    ["VM_IOREMAP",  0x00000001],      /* ioremap() and friends */
    ["VM_ALLOC",    0x00000002],      /* vmalloc() */
    ["VM_MAP",      0x00000004],      /* vmap()ed pages */
    ["VM_USERMAP",  0x00000008],      /* suitable for remap_vmalloc_range */
    ["VM_VPAGES",   0x00000010],      /* buffer for pages was vmalloc'ed */
    ["VM_UNLIST",   0x00000020]       /* vm_struct is not listed in vmlist */
];


function flagsToStr(flags)
{
    if (flags.isString())
        flags = parseInt(flags, 10);
    for (var i in flags_list) {
        if (flags == flags_list[i][1])
            return flags_list[i][0];
    }
    return "0x" + flags.toString(16);
}


function printVmAreas()
{
    var item = new Instance("vmlist");

    if (!item.IsValid()) {
        throw "Could not find variable 'vmlist'";
	}

    c_addr = 2 + (item.SizeofPointer() * 2);

    var tmp = new Instance("vmlist");

    while (!item.IsNull()) {
        var size = item.size - 4096;
        var addr_start = item.addr.toPointer();
        tmp.SetAddress(addr_start);
        tmp.AddToAddress(size - 1);
        var addr_end = tmp.Address();
        var type = flagsToStr(item.flags.toLong());
        var io = item.phys_addr.toString() != "0" ? "(0x" + item.phys_addr.toPointer() + ")" : "";

		var line =            
            ralign("0x" + addr_start, c_addr) +
            " - " +
            ralign("0x" + addr_end, c_addr) +
            ", " +
            ralign(size, c_size) +
            " " +
            lalign(type, c_type) +
            " " + io;

        println(line);
		
        item = item.next;
    }
}

printVmAreas();

