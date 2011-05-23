/*
 * created by diekmann on Mon 13 May 2011
 *
 * get the task which is currently active on a cpu
 * input parameter: GS_BASE register of given cpu
 *
 *
 * Technical background: get_current()
 * On linux 2.6.11 i386 as described in "understanding the linux kernel", get_current is
 * static inline struct task_struct * get_current(void)
 *         return current_thread_info()->task;
 * 
 * current_thread_info() uses the %esp register to get information about the current thread
 * 
 * On x86_64 the %gs register is used as segment selector like
 * 	mov %gs:<magic>, %rax
 * to retrieve information about the current task.
 * This still holds for 2.6.32 and possibly newer
 * 
 * <magic> is "per_cpu__current_task"
 * 
 * get current in the guestVM:
 *	rdmsrl(MSR_GS_BASE, gs_base);
 * 	void **addr = gs_base + <magic>
 * 	struct task_struct *current = *addr
 * 	the kernel macro for current is equal to asm("mov %%gs:0xcbc0, %0": "=r" (current));
 * 
 */

//always work with numbers as strings!!!
function getCurrentTask(gs_base)
{
	if (gs_base == 0) {
		return;
	}
	
	
	// laod with temporary symbol
	var current_task = new Instance("init_task")
	current_task.ChangeType("uint64_t")
	
	
	current_task.SetAddress(gs_base)
	
	// current_task.Address() is the address of the beginning of the segment pointed to by the gs register
	//print(current_task.Address())
	
	var offset = new Instance("per_cpu__current_task")
	//print(offset.Address())
	
	var addrLow = parseInt(offset.Address(), 16)
	
	current_task.AddToAddress(addrLow)
	
	// current_task.Address() now contains the Adress of the memory are where gs:per_cpu__current_task points
	//print(current_task.Address())
	
	// current_task now contains the Adress of the struct task_struct currently executing on the cpu
	//print(current_task)
	
	// convert current_task from its uint64_t decimal representation to hexadecimal representation
	// only work on bytes here becaus javascript cannot handle unsigne 64 bit integers!
	var current_task_hex = ""
	for(var i = 0; i < 8; ++i){
		var addrLow = current_task.toUInt8()
		addrLow = parseInt(addrLow).toString(16)
		current_task_hex = addrLow + current_task_hex
		if(addrLow.length == 1){
			current_task_hex = "0" + current_task_hex
		}
		current_task.AddToAddress(1)
	}
	
	// current_task_hex now contains the adress of the struct task_struct currently executing
	//print(current_task_hex)
	
	current_task.ChangeType("task_struct")
	current_task.SetAddress(current_task_hex)
	//print(current_task)
	
	return current_task
}

/* 
 * usage:
 * getCurrentTask("0xffff880001800000"); // %gs register hardcoded as string!
 */
