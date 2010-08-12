
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
	print();
	print("------------------------------------------------------------------------");
	print("TEST: " + title);
	print("------------------------------------------------------------------------");
}

//------------------------------------------------------------------------------
print("Creating instance of \"init_task.children\"");
var inst = new Instance("init_task.children");

//------------------------------------------------------------------------------
newTestSection("Calling prototype functions");

print("inst.IsNull() = " + inst.IsNull());
print("inst.Address() = 0x" + inst.Address());
print("inst.Name() = " + inst.Name());
print("inst.ParentName() = " + inst.ParentName());
print("inst.FullName() = " + inst.FullName());
//print("inst.MemberNames() = " + inst.MemberNames());
//print("inst.Members() = " + inst.Members());
//print("inst.Type() = " + inst.Type());
print("inst.TypeName() = " + inst.TypeName());
print("inst.Size() = " + inst.Size());
print("inst.MemberExists(\"foo\") = " + inst.MemberExists("foo"));
print("inst.MemberExists(\"pid\") = " + inst.MemberExists("pid"));
print("inst.FindMember(\"foo\").IsNull() = " + inst.FindMember("foo").IsNull());
print("inst.FindMember(\"pid\").IsNull() = " + inst.FindMember("pid").IsNull());
print("inst.TypeIdOfMember(\"foo\") = 0x" + inst.TypeIdOfMember("foo").toString(16));
print("inst.TypeIdOfMember(\"pid\") = 0x" + inst.TypeIdOfMember("pid").toString(16));
print("inst.toString() = " + inst.toString());

//------------------------------------------------------------------------------
newTestSection("Iterating over inst.MemberNames()");

var i;
var names = inst.MemberNames();
for (i = 0; i < names.length; ++i)
	print ((i+1) + ". member: " + names[i] + 
			", type = \"" + inst[names[i]].TypeName() + "\"" + 
			", size = " + inst[names[i]].Size() + " byte");

//------------------------------------------------------------------------------
newTestSection("Iterating over inst.Members()")

var members = inst.Members();
for (i = 0; i < members.length; ++i)
	print ((i+1) + ". member: " + members[i].FullName() + " @ 0x" + members[i].Address());

//------------------------------------------------------------------------------
newTestSection("Iterating over properties with iterator")

i = 1;
for (m in inst)
	print ((i++) + ". member: " + inst[m].FullName() + " @ 0x" + inst[m].Address());

//------------------------------------------------------------------------------
newTestSection("Invoking properties by string");

var m = ["next", "prev"];
for (i in m)
	print("inst[\"" + m[i] + "\"].FullName() = " + inst[m[i]].FullName() + " @ 0x" + inst[m[i]].Address());

//------------------------------------------------------------------------------
newTestSection("Invoking properties directly");

print("inst.next.FullName() = " + inst.next.FullName() + " @ 0x" + inst.next.Address());
print("inst.prev.FullName() = " + inst.prev.FullName() + " @ 0x" + inst.prev.Address());

//------------------------------------------------------------------------------
newTestSection("Member assignment and invokation with iterator");

i = 1;
for (m in inst) {
	var member = inst[m];
	print ((i++) + ". member: " + member.FullName() + " @ 0x" + member.Address());
}

//------------------------------------------------------------------------------
newTestSection("Member assignment and invokation directly from properties");

i = 1;
var member1 = inst.next;
var member2 = inst.next;
print ((i++) + ". member: " + member1.FullName() + " @ 0x" + member1.Address());
print ((i++) + ". member: " + member2.FullName() + " @ 0x" + member2.Address());


//------------------------------------------------------------------------------
newTestSection("Iterating over a member's members");

for (m in inst.next.sibling)
	print("inst.next.sibling[" + m + "].FullName() = " + inst.next.sibling[m].FullName());

//------------------------------------------------------------------------------
newTestSection("Testing instanceof operator");

print("inst instanceof \"Instance\" = " + (inst instanceof Instance));
print("member1 instanceof \"Instance\" = " + (member1 instanceof Instance));
print("member2 instanceof \"Instance\" = " + (member2 instanceof Instance));


//------------------------------------------------------------------------------
newTestSection("Testing address operations");
var w = 50;
var save = inst.Address();
print(lalign("inst.Address() = ", w) + ralign("0x" + inst.Address(), 18));
i = 16;
inst.AddToAddress(i);
print(lalign("Adding offset 0x" + i.toString(16) + " => ", w) + ralign("0x" + inst.Address(), 18));
i = -8;
inst.AddToAddress(i);
print(lalign("Adding offset 0x" + i.toString(16) + " => ", w) + ralign("0x" + inst.Address(), 18));
inst.AddToAddress(i);
print(lalign("Adding offset 0x" + i.toString(16) + " => ", w) + ralign("0x" + inst.Address(), 18));
i = 0xcafebabe;
inst.SetAddressHigh(i);
print(lalign("Setting address high to 0x" + i.toString(16) + " => ", w) + ralign("0x" + inst.Address(), 18));
i = 0xdeadbeef;
inst.SetAddressLow(i);
print(lalign("Setting address low to 0x" + i.toString(16) + " => ", w) + ralign("0x" + inst.Address(), 18));
var s = "0123456789abcdef";
inst.SetAddress(s);
print(lalign("Setting address to \"" + s + "\" => ", w) + ralign("0x" + inst.Address(), 18));
s = "0xfedcba9876543210";
inst.SetAddress(s);
print(lalign("Setting address to \"" + s + "\" => ", w) + ralign("0x" + inst.Address(), 18));
try {
	s = "Hello, world!";
	inst.SetAddress(s);
	print(lalign("Setting address to \"" + s + "\" => ", w) + ralign("0x" + inst.Address(), 18));
}
catch (e) {
	print(lalign(e + " => ", w) + ralign("0x" + inst.Address(), 18));
}
inst.SetAddress(save);
print(lalign("Restoring original address => ", w) + ralign("0x" + inst.Address(), 18));


