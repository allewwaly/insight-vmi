/*
 * created by diekmann on Mon 16 May 2011
 *
 * print the task which is currently active on a cpu
 * input parameter: GS_BASE register of given cpu
 *
 *
 * 
 */
 
include("lib_getCurrent_2.6.32-x64.js")

//always work with numbers as strings!!!
function printCurrentTask(gs_base)
{
	current_task = getCurrentTask(gs_base)
	
	try{
        println(current_task.cred.uid.toString());
	}catch(e){
        println("unknown");
	}
	try{
        println(current_task.cred.euid.toString());
	}catch(e){
        println("unknown");
	}
    println(current_task.pid.toString());
	try{
        println(current_task.cred.gid.toString());
	}catch(e){
        println("unknown");
	}
	try{
        println(current_task.cred.egid.toString());
	}catch(e){
        println("unknown");
	}
    println(current_task.state.toString());
    println(current_task.comm.toString());
    println("0x" + current_task.Address());
	
	
}

try{
	printCurrentTask(GS_BASE_2632x64); // %gs register hardcoded as string!
}catch(e){
    println("Exception in printCurrentTask")
    println(e)
	// pipe exceptions to caller, as there shouldn't be any
	throw(e)
}

