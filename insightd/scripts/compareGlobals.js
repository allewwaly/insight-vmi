
// Prints a string with a given width left-aligned
function lalign(s, len)
{
	while (len > 0 && s.length < len)
		s += " ";
	return s;
}

//Prints a string with a given width right-aligned
function ralign(s, len)
{
	while (len > 0 && s.length < len)
		s = " " + s;
	return s;
}


function myCompare(inst1, inst2)
{
	if (inst1.Equals(inst2))
	{
		// Do nothing
    	}
    	else
    	{
            println("Different: [" + inst1.MemDumpIndex() + "] " + inst1.FullName() +
              		", [" + inst2.MemDumpIndex() + "] " + inst2.FullName());
	}
} 



// Get Globals
var globals = getVariableIds();

// Vars
var tmpInst1;
var tmpInst2;

// Iterate over all globals
for (var i = 0; i < globals.length; ++i) 
{
	// Create an instance of the var in each dump
	tmpInst1 = new Instance(globals[i], 0);
	tmpInst2 = new Instance(globals[i], 1);
	
	// Compare
	myCompare(tmpInst1, tmpInst2);
}
