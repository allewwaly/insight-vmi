include("lib_hexStringArith.js")

function __dumpStringKernel(instance, limit){
	ret = "";
	var i;
	for(i=0; i<limit; ++i){
		var thisChar = parseInt(instance.toString(), 10)
		ret += String.fromCharCode(thisChar)
		instance.AddToAddress(1)	
	}
	return ret;
}


/**
 * returns string with the name of the file which is mapped in task_struct's address space at address
 * returns empty string if no mapping exists for address
 */
function getMemoryMap(task_struct, address, userPGD){
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
				
				
				ret += it.vm_start + " " + it.vm_end + " ";
				ret += __dumpStringKernel(tmp, 32);
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


include("lib_getCurrent_2.6.32-x64.js")
var mmap = getMemoryMap(getCurrentTask(GS_BASE_2632x64), 123)
print(mmap)
