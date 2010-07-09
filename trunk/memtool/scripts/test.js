
function newTestSection(title)
{
	print();
	print("------------------------------------------------------------------------");
	print("TEST: " + title);
	print("------------------------------------------------------------------------");
}

print("Creating instance of \"init_task.children\"");
var inst = new Instance("init_task.children");

//------------------------------------------------------------------------------
newTestSection("Calling prototype functions");

print("inst.isNull() = " + inst.isNull());
print("inst.address() = 0x" + inst.address().toString(16));
print("inst.name() = " + inst.name());
print("inst.parentName() = " + inst.parentName());
print("inst.fullName() = " + inst.fullName());
//print("inst.memberNames() = " + inst.memberNames());
//print("inst.members() = " + inst.members());
//print("inst.type() = " + inst.type());
print("inst.typeName() = " + inst.typeName());
print("inst.size() = " + inst.size());
print("inst.memberExists(\"foo\") = " + inst.memberExists("foo"));
print("inst.memberExists(\"pid\") = " + inst.memberExists("pid"));
print("inst.findMember(\"foo\").isNull() = " + inst.findMember("foo").isNull());
print("inst.findMember(\"pid\").isNull() = " + inst.findMember("pid").isNull());
print("inst.typeIdOfMember(\"foo\") = 0x" + inst.typeIdOfMember("foo").toString(16));
print("inst.typeIdOfMember(\"pid\") = 0x" + inst.typeIdOfMember("pid").toString(16));
print("inst.toString() = " + inst.toString());

//------------------------------------------------------------------------------
newTestSection("Iterating over inst.memberNames()");

var names = inst.memberNames();
for (var i = 0; i < names.length; ++i)
	print ((i+1) + ". member: " + names[i]);

//------------------------------------------------------------------------------
newTestSection("Iterating over inst.members()")

var members = inst.members();
for (var i = 0; i < members.length; ++i)
	print ((i+1) + ". member: " + members[i].fullName() + " @ 0x" + members[i].address().toString(16));

//------------------------------------------------------------------------------
newTestSection("Iterating over properties with iterator")

i = 1;
for (m in inst)
	print ((i++) + ". member: " + inst[m].fullName() + " @ 0x" + inst[m].address().toString(16));

//------------------------------------------------------------------------------
newTestSection("Invoking properties by string");

var m = ["next", "prev"];
for (i in m)
	print("inst[\"" + m[i] + "\"].fullName() = " + inst[m[i]].fullName() + " @ 0x" + inst[m[i]].address().toString(16));

//------------------------------------------------------------------------------
newTestSection("Invoking properties directly");

print("inst.next.fullName() = " + inst.next.fullName() + " @ 0x" + inst.next.address().toString(16));
print("inst.prev.fullName() = " + inst.prev.fullName() + " @ 0x" + inst.prev.address().toString(16));

//------------------------------------------------------------------------------
newTestSection("Member assignment and invokation with iterator");

i = 1;
for (m in inst) {
	var member = inst[m];
	print ((i++) + ". member: " + member.fullName() + " @ 0x" + member.address().toString(16));
}

//------------------------------------------------------------------------------
newTestSection("Member assignment and invokation directly from properties");

i = 1;
var member1 = inst.next;
var member2 = inst.next;
print ((i++) + ". member: " + member1.fullName() + " @ 0x" + member1.address().toString(16));
print ((i++) + ". member: " + member2.fullName() + " @ 0x" + member2.address().toString(16));


//------------------------------------------------------------------------------
newTestSection("Iterating over a member's members");

for (m in inst.next.sibling)
	print("inst.next.sibling[" + m + "].fullName() = " + inst.next.sibling[m].fullName());

//------------------------------------------------------------------------------
newTestSection("Testing instanceof operator");

print("inst instanceof \"Instance\" = " + (inst instanceof Instance));
print("member1 instanceof \"Instance\" = " + (member1 instanceof Instance));
print("member2 instanceof \"Instance\" = " + (member2 instanceof Instance));




