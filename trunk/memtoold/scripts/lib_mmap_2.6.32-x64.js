include("lib_hexStringArith.js")

function __dumpStringKernel(instance, limit){
	ret = "";
	var i;
	for(i=0; i<limit; ++i){
		var thisChar = parseInt(instance.toString(), 10)
		if(thisChar == 0) break;
		ret += String.fromCharCode(thisChar)
		instance.AddToAddress(1)
	}
	return ret;
}


/**
 * returns string representation of flags
 */
function __getVMAreaFlags(flags){
	var ret = "";
	if(flags & 0x00000001) ret += "VM_READ ";
	if(flags & 0x00000002) ret += "VM_WRITE ";
	if(flags & 0x00000004) ret += "VM_EXEC ";
	if(flags & 0x00000008) ret += "VM_SHARED ";
	return ret;	
}

/**
 * returns the mmapped areas for task_struct
 */
function getMemoryMap(task_struct){
	var ret = "";
	try{
		//active_mm
		var it = task_struct.mm.mmap;
		
		//TODO memtool IsNull Bug
		//while(!it.IsNull()){
		while(typeof(it) != "undefined"){
			var file = it.vm_file;
			
			//TODO
			if(typeof(file) == "undefined") break;
			
			
			if(file.toString() != "NULL"){
				file.ChangeType("file");
				var dentry = file.f_path.dentry;
				
				//TODO workaround
				//memtool unsigned char bug
				//TODO
				var tmp = dentry.d_iname
				tmp.ChangeType("char");
				//TODO address of d_iname calculated by hand!!
				tmp.SetAddress(__hex_add(dentry.Address(), "a0"));
				
				var mmappedFile = __dumpStringKernel(tmp, 32);
				//ret += it.vm_start + " " + it.vm_end + " ";
				//ret += mmappedFile;
				//ret += "\n"
				
				ret += __uintToHex(it.vm_start) + " " + __uintToHex(it.vm_end) + " ";
				ret += __getVMAreaFlags(it.vm_flags) + " ";
				ret += mmappedFile;
				ret += "\n"
				
			}
			it = it.vm_next;
		};
		
	}catch(e){
		ret += e;
		throw(e)
	}
	return ret;
}


//include("lib_getCurrent_2.6.32-x64.js")
//var mmap = getMemoryMap(getCurrentTask(GS_BASE_2632x64), 123)
//print(mmap)
