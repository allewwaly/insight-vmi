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
	print(getCurrentTask(gs_base));
	
}

try{
	printCurrentTask("0xffff880001800000"); // %gs register hardcoded as string!
}catch(e){
	print("Exception in printCurrentTask")
	print(e)
	// pipe exceptions to caller, as there shouldn't be any
	throw(e)
}

