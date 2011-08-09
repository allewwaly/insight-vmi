
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