/*
 * created by chrschn on Wed 22 May 2011
 *
 */

function printModuleList()
{
    var head = new Instance("modules");

    if (head.MemberCandidatesCount("next") <= 1)
        throw new Error("\"" + head.Name() + "\" does not have any candidate types for member \"next\"");

    var m = head.MemberCandidate("next", 0);
    print(m.name.toString());

	while (m && m.list.next.Address() != head.Address()) {
		m = m.list.MemberCandidate("next", 0);
		print(m.name.toString());
	}
}

printModuleList();
