/*
 * Copyright (C) 2013 by Christian Schneider <chrschn@sec.in.tum.de>
 */

/**
 * @param v instance of a module variable
 */
function module_var_in_section(v)
{
	if (!v || !v.IsValid())
		return v;
	
	var section_name = v.Section();
	// We require a section
	if (!section_name)
		throw "Variable '" + v.Name() + "' does not have a section.";
	
	// We can't resolve the ".modinfo" and "__versions" sections
	if (section_name == ".modinfo" || section_name == "__versions")
		return new Instance();

	// Extract module name
	var fileName = v.OrigFileName();
	if (fileName == "vmlinux")
		throw("Variable '" + v.Name() + "' belongs to the kernel.");
	var moduleName = fileName.substring(fileName.lastIndexOf("/")+1, fileName.lastIndexOf("."));
	// Underscores in module names are dashes in file names
	moduleName = moduleName.replace(/-/g, "_");

	// Find the corresponding module structure
	var modules = new Instance("modules");
	var m = modules.next;
	
	// The rule engine is disabled here, so we have to do the pointer magic by hand
	m.ChangeType("module");
	var offset = m.MemberOffset("list");
	var typeId = m.TypeId();
	
	var found = false;
	while (modules.Address() != m.Address()) {
		m.AddToAddress(-offset);
		if (m.name == moduleName) {
			found = true;
			break;
		}
		m = m.list.next;
		m.ChangeType(typeId);
	}

	// Return in invalid instance if the module is not loaded.
	if (!found)
		return new Instance();

	// Find the section with the given name
	var cnt = m.sect_attrs.nsections;
	var attrs = m.sect_attrs.attrs;
	for (var i = 0; i < cnt; ++i) {
		var attr = attrs.ArrayElem(i);
		if (attr.name == section_name) {
			v.AddToAddress(attr.address.toLong(16));
			return v;
		}
	}

	// We should not come here
	throw("Section '" + section_name + "' not found for module '" + moduleName + "'.");
}
