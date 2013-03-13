// Memory Range
var page_size = 4096;
var start = 1024 * 1024 * 1024 * 3; // 3Gb
var end = 1024 * 1024 * 1024 * 4; // 4Gb
var pages = (end - start) / page_size;

// Kernel Code Area
var kernel_code_start = 0xc0000000;// 0xc0100000; // _text
var kernel_code_end = 0xc05ff000;// 0xc032e869; // _etext
var kernel_vsyscall_page = 0xffffe000;

// Use init_task here to create valid instance
var page = new Instance("init_task");
// We start from start
page.SetAddress(start.toString(16));
// Change type to void *
// page.ChangeType(0x1ea);

// For ioremaped regions
var vmlist = new Instance("vmlist");

// Counter
var i = 0;
var j = 0;
var executable_pages = 0;
var non_executable_pages = 0;
var kernel_code_pages = 0;
var module_code_pages = 0;
var found = false;
var unmapped_code_pages = 0;

// Print Data
println("Trying to map kernel code pages using the following values:");
println("\t -> Start Address: 0x" + start.toString(16));
println("\t -> End Address: 0x" + end.toString(16));
println("\t -> Kernel Start Address: 0x" + kernel_code_start.toString(16));
println("\t -> Kernel End Address: 0x" + kernel_code_end.toString(16));
println();

// Parse Modules
var module_head = new Instance("modules");
var m = module_head.next;
var module_list = new Array();

while (m &&
           m.Address() != module_head.Address() &&
           m.MemberAddress("list") != module_head.Address())
{
    module_list[i] = new Object();
    module_list[i]["name"] = m.name;
    module_list[i]["core"] = m.module_core;
    module_list[i]["size"] = m.core_size;
    module_list[i]["found"] = 0;
    
    m = m.list.next;
    i += 1;
}

// Map Pages
for (i = 0; i < pages; ++i)
{   
    if (page.IsExecutable())
    {
	found = false;
        executable_pages++;
        
        // Wihtin Kernel Code Pages?
        if (parseInt(page.Address(), 16) >= kernel_code_start &&
            parseInt(page.Address(), 16) <= kernel_code_end)
	{
	    found = true;
            kernel_code_pages++;
	}
        
        // Within a module?
	if (!found)
        {
		for (j = 0; j < module_list.length; ++j)
        	{
            		if (parseInt(page.Address(), 16) >= parseInt(module_list[j]["core"], 16) &&
                	    parseInt(page.Address(), 16) <= (parseInt(module_list[j]["core"], 16) + parseInt(module_list[j]["size"])))
            		{
			found = true;
                	module_list[j]["found"] += 1;
                	break;
            		}
        	}
	}

	// Vsyscall Page?
	if (!found && parseInt(page.Address(), 16) == kernel_vsyscall_page)
	    found = true;

	// Check for ioremaped page
	if (!found)
	{
		for (j = vmlist; j && parseInt(vmlist.addr, 16) <= parseInt(page.Address(), 16); j = j.next)
		{
			if (parseInt(j.addr, 16) <= parseInt(page.Address(), 16) && 
		    	   (parseInt(j.addr, 16) + parseInt(j.size)) > parseInt(page.Address(), 16))
			{
				if (parseInt(j.flags) == 1)
					found = true;
				break;
			}	
		}
	}

	// Do not know that one
	if (!found)
	{
	    println("UNKNOWN CODE PAGE @ " + page.Address() + "!");
	    unmapped_code_pages++;
	}
        
    }
    else
    {
	non_executable_pages++;
    }
    
    page.AddToAddress(page_size);
}


/*
println("Found " + executable_pages + " executable pages and " + non_executable_pages + " non-executable/non-present pages.");
println("\t-> " + kernel_code_pages + " of these are kernel core code pages. (Expected: " + 
        Math.floor(((kernel_code_end - kernel_code_start) / page_size) + 1) + ")");


for (i = 0; i < module_list.length; ++i)
{
    println("\t-> " + module_list[i]["found"] + " belong to module " + module_list[i]["name"] +
            " (Expected: " + Math.floor((parseInt(module_list[i]["size"]) / page_size) + 1) + ")");
}
*/

if (unmapped_code_pages)
{
	println("WARNING: Detected " + unmapped_code_pages + " code pages that could not be mapped!");
}
else
{
	println("Successfully mapped ALL " + executable_pages + " executable pages!");
}
