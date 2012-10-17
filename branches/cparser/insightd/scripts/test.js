
function lalign(s, len)
{
	while (len > 0 && s.length < len)
		s += " ";
	return s;
}

//------------------------------------------------------------------------------
function ralign(s, len)
{
	while (len > 0 && s.length < len)
		s = " " + s;
	return s;
}

//------------------------------------------------------------------------------
function newTestSection(title)
{
    println();
    println("------------------------------------------------------------------------");
    println("TEST: " + title);
    println("------------------------------------------------------------------------");
}

//------------------------------------------------------------------------------
println("Creating instance of \"init_task.children\"");
var inst = new Instance("init_task.children");

//------------------------------------------------------------------------------
newTestSection("Calling prototype functions");

println("inst.IsNull() = " + inst.IsNull());
println("inst.Address() = 0x" + inst.Address());
println("inst.Name() = " + inst.Name());
println("inst.ParentName() = " + inst.ParentName());
println("inst.FullName() = " + inst.FullName());
//println("inst.MemberNames() = " + inst.MemberNames());
//println("inst.Members() = " + inst.Members());
//println("inst.Type() = " + inst.Type());
println("inst.TypeName() = " + inst.TypeName());
println("inst.Size() = " + inst.Size());
println("inst.MemberExists(\"foo\") = " + inst.MemberExists("foo"));
println("inst.MemberExists(\"pid\") = " + inst.MemberExists("pid"));
println("inst.Member(\"foo\").IsNull() = " + inst.Member("foo"));
println("inst.Member(\"pid\").IsNull() = " + inst.Member("pid"));
println("inst.TypeIdOfMember(\"foo\") = 0x" + inst.TypeIdOfMember("foo").toString(16));
println("inst.TypeIdOfMember(\"pid\") = 0x" + inst.TypeIdOfMember("pid").toString(16));
println("inst.toString() = " + inst.toString());

//------------------------------------------------------------------------------
newTestSection("Iterating over inst.MemberNames()");

var i;
var names = inst.MemberNames();
for (i = 0; i < names.length; ++i)
    println ((i+1) + ". member: " + names[i] +
			", type = \"" + inst[names[i]].TypeName() + "\"" + 
			", size = " + inst[names[i]].Size() + " byte");

//------------------------------------------------------------------------------
newTestSection("Iterating over inst.Members()")

var members = inst.Members();
for (i = 0; i < members.length; ++i)
    println ((i+1) + ". member: " + members[i].FullName() + " @ 0x" + members[i].Address());

//------------------------------------------------------------------------------
newTestSection("Iterating over properties with iterator")

i = 1;
for (m in inst)
    println ((i++) + ". member: " + inst[m].FullName() + " @ 0x" + inst[m].Address());

//------------------------------------------------------------------------------
newTestSection("Invoking properties by string");

var m = ["next", "prev"];
for (i in m)
    println("inst[\"" + m[i] + "\"].FullName() = " + inst[m[i]].FullName() + " @ 0x" + inst[m[i]].Address());

//------------------------------------------------------------------------------
newTestSection("Invoking properties directly");

println("inst.next.FullName() = " + inst.next.FullName() + " @ 0x" + inst.next.Address());
println("inst.prev.FullName() = " + inst.prev.FullName() + " @ 0x" + inst.prev.Address());

//------------------------------------------------------------------------------
newTestSection("Member assignment and invokation with iterator");

i = 1;
for (m in inst) {
	var member = inst[m];
    println ((i++) + ". member: " + member.FullName() + " @ 0x" + member.Address());
}

//------------------------------------------------------------------------------
newTestSection("Member assignment and invokation directly from properties");

i = 1;
var member1 = inst.next;
var member2 = inst.next;
println ((i++) + ". member: " + member1.FullName() + " @ 0x" + member1.Address());
println ((i++) + ". member: " + member2.FullName() + " @ 0x" + member2.Address());


//------------------------------------------------------------------------------
newTestSection("Iterating over a member's members");

for (m in inst.next.sibling)
    println("inst.next.sibling[" + m + "].FullName() = " + inst.next.sibling[m].FullName());

//------------------------------------------------------------------------------
newTestSection("Testing instanceof operator");

println("inst instanceof \"Instance\" = " + (inst instanceof Instance));
println("member1 instanceof \"Instance\" = " + (member1 instanceof Instance));
println("member2 instanceof \"Instance\" = " + (member2 instanceof Instance));


//------------------------------------------------------------------------------
newTestSection("Testing address operations");
var w = 50;
var save = inst.Address();
println(lalign("inst.Address() = ", w) + ralign("0x" + inst.Address(), 18));
i = 16;
inst.AddToAddress(i);
println(lalign("Adding offset 0x" + i.toString(16) + " => ", w) + ralign("0x" + inst.Address(), 18));
i = -8;
inst.AddToAddress(i);
println(lalign("Adding offset 0x" + i.toString(16) + " => ", w) + ralign("0x" + inst.Address(), 18));
inst.AddToAddress(i);
println(lalign("Adding offset 0x" + i.toString(16) + " => ", w) + ralign("0x" + inst.Address(), 18));
i = 0xcafebabe;
inst.SetAddressHigh(i);
println(lalign("Setting address high to 0x" + i.toString(16) + " => ", w) + ralign("0x" + inst.Address(), 18));
i = 0xdeadbeef;
inst.SetAddressLow(i);
println(lalign("Setting address low to 0x" + i.toString(16) + " => ", w) + ralign("0x" + inst.Address(), 18));
var s = "0123456789abcdef";
inst.SetAddress(s);
println(lalign("Setting address to \"" + s + "\" => ", w) + ralign("0x" + inst.Address(), 18));
s = "0xfedcba9876543210";
inst.SetAddress(s);
println(lalign("Setting address to \"" + s + "\" => ", w) + ralign("0x" + inst.Address(), 18));
try {
	s = "Hello, world!";
	inst.SetAddress(s);
    println(lalign("Setting address to \"" + s + "\" => ", w) + ralign("0x" + inst.Address(), 18));
}
catch (e) {
    println(lalign(e + " => ", w) + ralign("0x" + inst.Address(), 18));
}
inst.SetAddress(save);
println(lalign("Restoring original address => ", w) + ralign("0x" + inst.Address(), 18));


