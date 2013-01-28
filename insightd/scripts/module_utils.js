
include("scripts/lib_string.js"); 

function print_module_syms(m)
{
	println("===", m.name + ".module_core = 0x" + m.module_core.Address(), "===");
	
	var cnt = m.num_syms; 
	
	for (var i = 0; i < cnt; ++i) { 
		var sym = m.syms.ArrayElem(i); 
		println(lalign(sym.name, 30), " @ 0x" + sym.value.toULong(16));
	}
}


function print_module_sections(m)
{
	println("===", m.name + ".module_core = 0x" + m.module_core.Address(), "===");
	
	var attrs = m.sect_attrs;
	var cnt = attrs.nsections;
	
	for (var i = 0; i < cnt; ++i) { 
		var attr = attrs.attrs.ArrayElem(i);
		println(lalign(attr.name, 30), " @ 0x" + attr.address.toULong(16));
	}
}


function print_module_var(v, section_name)
{
	if (!v.IsValid())
		return v;
	
	// Extract module name
	var fileName = v.OrigFileName();
	if (fileName == "vlinux" || fileName == "")
		println(v);
	var moduleName = fileName.substring(fileName.lastIndexOf("/")+1, fileName.lastIndexOf("."));
	// Find the corresponding module structure
	var modules = new Instance("modules");
	var m = modules.next;
	while (modules.Address() != m.Address() && modules.Address() != m.list.Address()) {
		if (m.name == moduleName) {
			println("Found module: " + m.name);
			break;
		}
		m = m.list.next;		
	}
	
	// To be continued
}