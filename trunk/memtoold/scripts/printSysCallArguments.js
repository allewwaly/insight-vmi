if(HAVE_GLOBAL_PARAMS){
	eval("params="+PARAMS)
	var sys_call_nr = params["sys_call_nr"] //int
	var arg0 = params["rdi"] //hex string
	var arg1 = params["rsi"] //hex string
	var arg2 = params["rdx"] //hex string
	var arg3 = params["r10"] //hex string
	var arg4 = params["r8"] //hex string
	var arg5 = params["r9"] //hex string
	var userPGD = params["cr3"] // hex string
	
	if(typeof(arg0) == "undefined") print("rdi undefined")
	if(typeof(arg1) == "undefined") print("r2i undefined")
	if(typeof(arg2) == "undefined") print("rdx undefined")
	if(typeof(arg3) == "undefined") print("r10 undefined")
	if(typeof(arg4) == "undefined") print("r8 undefined")
	if(typeof(arg5) == "undefined") print("r9 undefined")
	if(typeof(userPGD) == "undefined") print("cr3 undefined")
}else{
	print("no parameters specified")
}


/*from entry_64.S
 432 * Register setup:
 433 * rax  system call number
 434 * rdi  arg0
 435 * rcx  return address for syscall/sysret, C arg3
 436 * rsi  arg1
 437 * rdx  arg2
 438 * r10  arg3    (--> moved to rcx for C)
 439 * r8   arg4
 440 * r9   arg5
 441 * r11  eflags for syscall/sysret, temporary for C
 442 * r12-r15,rbp,rbx saved by C code, not touched.
 443 *
 444 * Interrupts are off on entry.
 445 * Only called from user space.
 446 *
 447 * XXX  if we had a free scratch register we could save the RSP into the stack frame
 448 *      and report it properly in ps. Unfortunately we haven't.
 449 *
 450 * When user can change the frames always force IRET. That is because
 451 * it deals with uncanonical addresses better. SYSRET has trouble
 452 * with them due to bugs in both AMD and Intel CPUs.
 453 */



// dict generated by gen_syscall_memtool_script.py, available in diekmann's git
include("lib_syscalls_2.6.32-x64.js");

include("lib_getCurrent_2.6.32-x64.js");

include("lib_typecasting.js");

include("lib_mmap_2.6.32-x64.js");

//workaround to create an instance
//EMPTY_INSTANCE should signal that we do not care about the symbol
EMPTY_INSTANCE="init_task";


function main(){
	var tmpInst = new Instance(EMPTY_INSTANCE)
	tmpInst.ChangeType("uint64_t");
	tmpInst.SetAddress("0");
	
	print(SysCalls[sys_call_nr]["sys_name"]+": "+SysCalls[sys_call_nr]["signature"]);
	
	var line = "";
	
	// set this to true if we have strong indication that a dump of the current memory
	// mappings would be usefull to display
	var want_mmap_mappings = false;
	
	if(SysCalls[sys_call_nr]["sys_name"] == "sys_mmap" || 
		SysCalls[sys_call_nr]["sys_name"] == "sys_munmap" || 
		SysCalls[sys_call_nr]["sys_name"] ==  "sys_mremap" || 
		SysCalls[sys_call_nr]["sys_name"] ==  "sys_remap_file_pages") want_mmap_mappings = true;
	
	if(SysCalls[sys_call_nr]["signature"] == None){
		line += "unknown signature, dumping registers\n"
		for(i=0; i < 6; i++){
			var value = eval("arg"+i)
			line += "arg"+i+": 0x"+value+"\n";
		}
		print(line)
	}
	
	var i

	for(i=0; i < SysCalls[sys_call_nr]["argc"]; i++){
		var arg = SysCalls[sys_call_nr]["arg"+i];
		var next_arg = SysCalls[sys_call_nr]["arg"+(i+1)];
	
		// global parameter sys_call_arg is now the conetent of the register which contains the syscall parameter
		var sys_call_arg = eval("arg"+i) ;
	
		line = "\t";
	
		var arg_name = arg["name"] == "" ? "?unknown?" : arg["name"];
		line += arg_name+": ";
		
		tmpInstValid = false;
		if(!arg["isPtr"]){
			// treat argument as value in register, try to cast to specific type
			// and print it
			line += arg["type"]+": ";
	
			tmpInst.SetAddress("0"); // ignore addr
			try{
				__tryChangeType(tmpInst, arg["type"]);
				tmpInstValid = true;
			}catch(e){
				line += "exception: ";
				line += e;
			}

			if(tmpInstValid){
				line += "0x"+sys_call_arg;
			}
		}else{
			// treat argument as pointer, just print value
			line += arg["type"]+" "+arg["user_ptr"]+": 0x"+sys_call_arg;
			
			//try to deref the value
			try{
				__tryChangeType(tmpInst, arg["type"]);
				tmpInst.SetAddress(sys_call_arg);
				tmpInstValid = true;
			}catch(e){
				line += " cannot dereference: ";
				line += e;
			}

			if(tmpInstValid){
				try{
				
					var dereferenced = tmpInst.derefUserLand(userPGD);
					
					line += " -> \n\t\t"+dereferenced.replace(/\n/g, "\n\t\t");
					
					// now we try to output some nice humean readable representation if some arguments
					// are obvious
					if(arg["name"] == "filename"){
						line += " hex: ";
						var str_filename = "";
						for(var k = 0; k < 256; k++){
							var thisChar = parseInt(tmpInst.derefUserLand(userPGD), 10)
							if(thisChar == 0) break
							line += thisChar.toString(16)+" "
							str_filename += String.fromCharCode(thisChar)
							tmpInst.AddToAddress(1)
						}
						line += "string: "+str_filename
					}else if(arg["name"] == "buf" && next_arg["name"] == "count" && SysCalls[sys_call_nr]["sys_name"] != "sys_read"){
						var buffSize = parseInt(eval("arg"+(i+1)), 16) 
						line += "\nbuffer content hex (of size "+buffSize+"):\n";
						var str_buffer = "";
						for(var k = 0; k < buffSize; k++){
							var thisChar = parseInt(tmpInst.derefUserLand(userPGD), 10)
							line += thisChar.toString(16)+" "
							str_buffer += String.fromCharCode(thisChar)
							tmpInst.AddToAddress(1)
						}
						line += "\nbuffer content string: " + str_buffer
					}else if(arg["name"] == "argv" || arg["name"] == "envp"){
						//array, nullterminated of pointers to strings
					}
				}catch(e){
					line += " cannot dereference: ";
					line += e;
					
					want_mmap_mappings = true; //if this address was mmapped right before, the kernel 
					// performs the mapping lazy on first access to this region, so we want to see
					// at least the current mappings to check whether this access failed due to
					// lazy mapping
				}
				
			}
		}
		print(line)
	
	}
	if(want_mmap_mappings){
		line = "Memory mapping before syscall:\n";
		var mmap = getMemoryMap(getCurrentTask(GS_BASE_2632x64))
		line += mmap;
		print(line)
	}
	
}

try{
	main()
}catch(e){
	print("Exception in main():")
	print(e)
}
