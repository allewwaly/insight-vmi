
//var it = getInstance("init_task");
var it = new Instance("init_task");

print("it = \"" + it.typeName() + "\" @ 0x" + it.address().toString(16));
//print(it.toString());

//var names = it.memberNames();
//print("names = " + names);

//for (var members in it) {
//	print(members.toString());
//} 

// for (var i = 0; i < names.length; i++) {
//	print(names[i]);
//}


var m = it.members();
print("m = " + m);

for (var i = 0; i < m.length; i++) {
	print(m[i].name + ", " + m[i].size);
}