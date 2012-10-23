
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
    println("h: " + h + ", l: " + l +	", a: " + inst.Address() + e);

	if (arguments.length > 1) {
		expected = zeroPad(expected, 16);
		if (i.Address() != expected)
            println(">>>>>>>> Expected value was: " + expected + " <<<<<<<<");
	}
}

println("This scripts demonstrates the pointer arithmetic for 64 bit systems:");

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
