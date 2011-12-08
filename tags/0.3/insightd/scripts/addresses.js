
function zeroPad(s, width)
{
	while (s.length < width)
		s = "0" + s;
	return s;
}

function printAddr(i, expected)
{
	var h = zeroPad(inst.AddressHigh().toString(16), 8);
	var l = zeroPad(inst.AddressLow().toString(16), 8);
	var e = (h + l != inst.Address()) ? " <<< ERROR" : "";
	print("h: " + h + ", l: " + l +	", a: " + inst.Address() + e);

	if (arguments.length > 1) {
		expected = zeroPad(expected, 16);
		if (i.Address() != expected)
			print(">>>>>>>> Expected value was: " + expected + " <<<<<<<<");
	}
}

var inst = new Instance("init_task");
printAddr(inst);

// Reset address
inst.SetAddress("0");
printAddr(inst, "0");

// Setting high and low addresses
inst.SetAddressHigh(0xffffffff);
printAddr(inst, "ffffffff00000000");

inst.SetAddressLow(0xffffffff);
printAddr(inst, "ffffffffffffffff");

inst.SetAddressHigh(0);
printAddr(inst, "00000000ffffffff");

inst.SetAddressLow(0);
printAddr(inst, "0");

inst.SetAddressLow(0xfffffffe);
printAddr(inst, "00000000fffffffe");

// Adding to address, with 32-bit overflow
inst.AddToAddress(1);
printAddr(inst, "00000000ffffffff");

inst.AddToAddress(1);
printAddr(inst, "0000000100000000");

inst.AddToAddress(0);
printAddr(inst, "0000000100000000");

inst.AddToAddress(1);
printAddr(inst, "0000000100000001");

// Adding negative values, with 32-bit underflow
inst.AddToAddress(-1);
printAddr(inst, "0000000100000000");

inst.AddToAddress(-1);
printAddr(inst, "00000000ffffffff");

inst.AddToAddress(-1);
printAddr(inst, "00000000fffffffe");

// Provoking 64-bit overflow
inst.SetAddress("ffffffffffffffff");
printAddr(inst, "ffffffffffffffff");

inst.AddToAddress(1);
printAddr(inst, "0");

inst.AddToAddress(1);
printAddr(inst, "1");

//Provoking 64-bit underflow
inst.AddToAddress(-2);
printAddr(inst, "ffffffffffffffff");

//===================[ Testing code from lib_getCurrent ]=======================

// Only test for these constants if we have loaded the memory file these values
// were obtained from!
var files = getMemDumps();
var file = files[0].split("/");
var test_constants = (file[file.length-1] == "20110713-170436.bin") ? true : false;

// Setting to custom address (lib_getCurrent:40-45)
GS_BASE_2632x64 = "0xffff880001800000";
inst.SetAddress(GS_BASE_2632x64);
printAddr(inst, "ffff880001800000");

// Adding offset accordingly (lib_getCurrent:50-53)
var offset = new Instance("per_cpu__current_task")
inst.AddToAddress(offset.AddressLow());
if (test_constants)
	printAddr(inst, "ffff88000180cbc0");
else
	printAddr(inst);

// Dereferencing pointer (lib_getCurrent:63-72)
var inst_hex = inst.toUInt64(16);
inst.SetAddress(inst_hex);
if (test_constants)
	printAddr(inst, "ffffffff8144b1f0");
else
	printAddr(inst, inst_hex);

print("inst.comm =      " + inst.comm);
print("inst.pid  =      " + inst.pid);
print("inst.cred.uid  = " + inst.cred.uid);

var mm_it = inst.active_mm.mmap;
var i = 0;
while (!mm_it.IsNull()) {
	print(++i + ". Addr: 0x" + mm_it.Address());
	mm_it = mm_it.vm_next;
}
