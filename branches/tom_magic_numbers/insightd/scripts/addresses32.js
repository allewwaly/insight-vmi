
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
	var e = (l != inst.Address()) ? " <<< ERROR" : "";
	print("h: " + h + ", l: " + l +	", a: " + inst.Address() + e);

	if (arguments.length > 1) {
		expected = zeroPad(expected, 8);
		if (i.Address() != expected)
			print(">>>>>>>> Expected value was: " + expected + " <<<<<<<<");
	}
}

print("This scripts demonstrates the pointer arithmetic for 32 bit systems:");

var inst = new Instance("init_task");
printAddr(inst);

// Reset address
inst.SetAddress("0");
printAddr(inst, "0");

// Setting high and low addresses
inst.SetAddressHigh(0xffffffff);
printAddr(inst, "00000000");

inst.SetAddressLow(0xffffffff);
printAddr(inst, "ffffffff");

inst.SetAddressHigh(0);
printAddr(inst, "ffffffff");

inst.SetAddressLow(0);
printAddr(inst, "0");

inst.SetAddressLow(0xfffffffe);
printAddr(inst, "fffffffe");

// Adding to address, with 32-bit overflow
inst.AddToAddress(1);
printAddr(inst, "ffffffff");

inst.AddToAddress(1);
printAddr(inst, "00000000");

inst.AddToAddress(0);
printAddr(inst, "00000000");

inst.AddToAddress(1);
printAddr(inst, "00000001");

// Adding negative values, with 32-bit underflow
inst.AddToAddress(-1);
printAddr(inst, "00000000");

inst.AddToAddress(-1);
printAddr(inst, "ffffffff");

inst.AddToAddress(-1);
printAddr(inst, "fffffffe");

// Provoking 64-bit overflow
inst.SetAddress("ffffffffffffffff");
printAddr(inst, "ffffffff");

inst.AddToAddress(1);
printAddr(inst, "0");

inst.AddToAddress(1);
printAddr(inst, "1");

//Provoking 64-bit underflow
inst.AddToAddress(-2);
printAddr(inst, "ffffffff");
