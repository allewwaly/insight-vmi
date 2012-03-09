/*
 * created by chrschn on Wed 22 May 2011
 *
 */

include("lib_string.js");

function printModuleList()
{
    var head = new Instance("modules");

    if (head.MemberCandidatesCount("next") < 1)
        throw new Error("\"" + head.Name() + "\" does not have any candidate types for member \"next\"");

    var w_mod = 20;
    print(lalign("Module", w_mod) + " Used by");

	// Iterate over all modules
	var m = head.next;
	while (m && m.MemberAddress("list", true) != head.Address()) {
		// Build list of modules which use the current module
		var usedList = "";
		var u = m.modules_which_use_me.next;
		while (u.MemberAddress("list", true) != m.modules_which_use_me.Address()) {
			if (usedList.length > 0)
				usedList = usedList + ", ";
			usedList = usedList + u.module_which_uses.name.toString();
			u = u.list.next;
		}

		if (usedList.length > 0)
			print(lalign(m.name.toString(), w_mod) + " " + usedList);
		else
			print(m.name.toString());

		m = m.list.next;
	}
}

printModuleList();
