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
        println("UID   = " + current_task.cred.uid.toString());
	}catch(e){
        println("UID   = unknown");
	}
	try{
        println("EUID  = " + current_task.cred.euid.toString());
	}catch(e){
        println("EUID  = unknown");
	}
    println("PID   = " + current_task.pid.toString());
	try{
        println("GID   = " + current_task.cred.gid.toString());
	}catch(e){
        println("GID   = unknown");
	}
	try{
        println("EGID  = " + current_task.cred.egid.toString());
	}catch(e){
        println("EGID  = unknown");
	}
    println("STATE = " + current_task.state.toString());
    println("CMD   = "+current_task.comm.toString());
    println("ADDR  = 0x" + current_task.Address());
}

try{
	printCurrentTask(GS_BASE_2632x64); // %gs register hardcoded as string!
}catch(e){
    println("Exception in printCurrentTask")
    println(e)
	// pipe exceptions to caller, as there shouldn't be any
	throw(e)
}

