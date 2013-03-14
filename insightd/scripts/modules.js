/*
 * created by chrschn on Wed 22 May 2011
 *
 */

include("lib_string.js");

function printModuleListCand()
{
    var head = new Instance("modules");

    if (head.MemberCandidatesCount("next") < 1)
        throw new Error("\"" + head.Name() + "\" does not have any candidate types for member \"next\"");

    var w_mod = 20;
    println(lalign("Address", 18) + " " + lalign("Module [Args]", w_mod) + " Used by");

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

		var name = "0x" + lalign(m.Address().toString(16), 16);
		name += " ";
		name += m.name.toString();
		if (m.args.toString().length > 0)
			name += " [" + m.args.toString() + "]";

		if (usedList.length > 0)
            println(lalign(name, w_mod) + " " + usedList);
		else
            println(name);

		m = m.list.next;
	}
}

function printModuleListRules()
{
    var head = new Instance("modules");

    var w_mod = 20;
    println(lalign("Address", 18) + " " + lalign("Module [Args]", w_mod) + " Used by");

    // Iterate over all modules
    var m = head.next;
    while (m &&
           m.Address() != head.Address() &&
           m.MemberAddress("list") != head.Address())
    {
        // Build list of modules which use the current module
        var usedList = "";
        var u = m.modules_which_use_me.next;
        while (u.TypeId() == m.TypeId() &&
               u.MemberAddress("list", true) != m.modules_which_use_me.Address())
        {
            if (usedList.length > 0)
            usedList = usedList + ", ";
            usedList = usedList + u.module_which_uses.name.toString();
            u = u.list.next;
        }

        var name = "0x" + lalign(m.Address().toString(16), 16);
        name += " ";
        name += m.name.toString();
        if (m.args.toString().length > 0)
        name += " [" + m.args.toString() + "]";

        if (usedList.length > 0)
        println(lalign(name, w_mod) + " " + usedList);
        else
        println(name);

        m = m.list.next;
    }
}

printModuleListRules();
