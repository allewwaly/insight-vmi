
var it = getInstance("init_task");

print("it = " + it.toString());

var names = it.memberNames();
//print("names = " + names);

// for (var i = 0; i < names.length; i++) {
//	print(names[i]);
//}


var m = it.members();

for (var i = 0; i < m.length; i++) {
	print(m[i].name + ", " + m[i].size);
}