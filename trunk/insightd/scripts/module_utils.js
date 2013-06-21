/*
 * Copyright (C) 2013 by Christian Schneider <chrschn@sec.in.tum.de>
 */

include("scripts/lib_string.js"); 

function find_module(name)
{
    var head = new Instance("modules");

    // Iterate over all modules
    var m = head.next;
    while (m &&
           m.Address() != head.Address() &&
           m.MemberAddress("list") != head.Address())
    {
        if (m.name == name)
            return m;
        m = m.list.next;
    }

    return false;
}


function print_module_heading(m)
{
    print("===", m.name);
    if (m.MemberExists("module_core"))
        print(".module_core = 0x" + m.module_core.Address());
    println(" ===");
}


function print_module_syms(m)
{
    print_module_heading(m);

    var cnt = m.num_syms;
	
	for (var i = 0; i < cnt; ++i) { 
		var sym = m.syms.ArrayElem(i); 
		println(lalign(sym.name, 30), " @ 0x" + sym.value.toULong(16));
	}
}


function print_module_sections(m)
{
    print_module_heading(m);

	var attrs = m.sect_attrs;
	var cnt = attrs.nsections;
	
	for (var i = 0; i < cnt; ++i) { 
		var attr = attrs.attrs.ArrayElem(i);
		println(lalign(attr.name, 30), " @ 0x" + attr.address.toULong(16));
	}
}


function get_module_var(var_name, section_name)
{
	// Disable the rules engine
	var useRulesSave = Instance.useRules;
	Instance.useRules = false;
	var v = new Instance(var_name);
	
	if (!v.IsValid()) {
		Instance.useRules = useRulesSave;
		return v;
	}
	
	// Enable the engine, we need it from now on
	Instance.useRules = true;
	
	// Extract module name
	var fileName = v.OrigFileName();
	if (fileName == "vmlinux")
		throw("Variable '" + var_name + "' belongs to the kernel.");
	var moduleName = fileName.substring(fileName.lastIndexOf("/")+1, fileName.lastIndexOf("."));
	
	// Find the corresponding module structure
	var modules = new Instance("modules");
	var m = modules.next;
	var found = false;
	while (modules.Address() != m.Address() && modules.Address() != m.list.Address()) {
		if (m.name == moduleName) {
			found = true;
			break;
		}
		m = m.list.next;		
	}
	
	// Throw an error if we didn't find the module
	if (!found || !m.IsValid()) {
		Instance.useRules = useRulesSave;
		throw("Module '" + moduleName + "' not found or not loaded.");
	}
	
	// Find the section with the given name	
	found = false;
	var cnt = m.sect_attrs.nsections;
	var attrs = m.sect_attrs.attrs;
	for (var i = 0; i < cnt; ++i) {
		var attr = attrs.ArrayElem(i);
		if (attr.name == section_name) {
			v.AddToAddress(attr.address.toLong(16));
			found = true;
			break;
		}
	}
	
	Instance.useRules = useRulesSave;
	if (found)
		return v;
	else
		throw("Section '" + section_name + "' not found for module '" + moduleName + "'.");
}


function print_module_var(var_name, section_name)
{
	var i = get_module_var(var_name, section_name);
	println(i.Dereference());
}
