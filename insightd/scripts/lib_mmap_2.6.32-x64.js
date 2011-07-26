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
		var it = task_struct.active_mm.mmap;
		
		//TODO InSight IsNull Bug
		//while(!it.IsNull()){
		while(typeof(it) != "undefined"){
			var file = it.vm_file;
			
			//TODO
			if(typeof(file) == "undefined") break;
			
			ret += __uintToHex(it.vm_start) + " " + __uintToHex(it.vm_end) + " ";
			ret += __getVMAreaFlags(it.vm_flags) + " ";
			if(file.toString() != "NULL"){
				//TODO InSight bug:
				// change type to file although it is already file
				file.ChangeType("file");
				var dentry = file.f_path.dentry;
				
				//TODO workaround
				//InSight unsigned char bug
				//TODO
				var tmp = dentry.d_iname
				tmp.ChangeType("char");
				//TODO address of d_iname calculated by hand!!
				tmp.SetAddress(__hex_add(dentry.Address(), "a0"));
				
				var mmappedFile = __dumpStringKernel(tmp, 32);
				//ret += it.vm_start + " " + it.vm_end + " ";
				//ret += mmappedFile;
				//ret += "\n"
				
				
				
				ret += mmappedFile;
				
				
			}else{
				ret += "?";
			}
			ret += "\n"
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


/* returns the human readable flags of a mmap syscall of signature 
 * void *mmap(void *start, size_t length, int prot, int flags, int fd, off_t offset);
 * @param: flags from mmap
 */
function getMMAPsyscall_flags(flags){
	var ret = "";
	if(flags & 0x01) ret += "MAP_SHARED ";
	if(flags & 0x02) ret += "MAP_PRIVATE ";
	if(flags & 0x0f) ret += "MAP_TYPE ";
	if(flags & 0x10) ret += "MAP_FIXED ";
	if(flags & 0x20) ret += "MAP_ANONYMOUS ";
	if(flags & 0x4000000) ret += "MAP_UNINITIALIZED ";
	return ret;
}
