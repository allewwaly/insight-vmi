/**
 * returns string with the name of the file which is mapped in task_struct's address space at address
 * returns empty string if no mapping exists for address
 */
function getMemoryMap(task_struct, address){
	var ret = "";
	try{
		//active_mm
		var it = task_struct.mm.mmap;
		//var end = it;
		while(!it.IsNull()){
		//while(typeof(it) != "undefined"){
			//var file = it.vm_file
			print(it.Address())
			print(it)
			//print(file);			
			if(false && file.toString() != "NULL"){
				file.ChangeType("file")
				
				print("\""+file.toString()+"\"")
				print("\""+file.f_path.toString()+"\"")
				var dentry = file.f_path.dentry;
				print("dentry at: "+dentry.Address()+":")
				print(dentry)
				//print(dentry.d_iname.arrayElem(0))
				print(dentry.d_iname.Address())
				print(dentry.d_iname.TypeName())
				print(dentry.d_iname.Type())
				print(dentry.d_iname.Name())
				print(dentry.d_iname.toString()) // d_iname
				//print(it.vm_file)
				//print(file.Members())
				//print(file.Address())
				print("--")
			}
			it = it.vm_next;
		};
		
	}catch(e){
		ret += e;
	}
	return ret;
}

include("lib_getCurrent_2.6.32-x64.js")
var mmap = getMemoryMap(getCurrentTask(GS_BASE_2632x64), 123)
print(mmap)
