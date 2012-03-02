
function lalign(s, len)
{
	s = s.toString();
	while (len > 0 && s.length < len)
		s += " ";
	return s;
}


function ralign(s, len)
{
	s = s.toString();
	while (len > 0 && s.length < len)
		s = " " + s;
	return s;
}


function hline(len)
{
	var s = "";
	for (var i = 0; i < len; ++i)
		s += "-";
	return s;
}

// Returns an unsigned hex representation of the given integer
function uhex(i)
{
	// Positive integers can be converted directly
	if (i >= 0)
		return i.toString(16);
	// The least significant 28 bit are easy
	var back = i & 0x0fffffff;
	// Shift the most significant bits without sign extension
	var front = i >>> 28;
	return front.toString(16) + back.toString(16);
}
