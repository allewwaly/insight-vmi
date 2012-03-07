/*
 * created by chrschn on Wed 22 May 2011
 *
 */

function printModuleList()
{
	var head = new Instance("modules");
	var m = head.next;
	m.ChangeType("module");	

	// Offset we have to use for correction
	var offset = m.MemberOffset("list");
	m.AddToAddress(-offset);

	// Prepare head for correct loop terminaten
	head.AddToAddress(-offset);
	
	do {
		print(m.name.toString());
		// Automated through source code parser
		if (m.list.MemberCandidatesCount("next") > 0) {
			m = m.list.MemberCandidate("next", 0);
		}
		// Manually adjust type and offset work
		else {
			// Maybe throw an exception here?
			m = m.list.next;
			m.ChangeType("module");
			m.AddToAddress(-offset);
		}
	} while (m && m.Address() != head.Address());
}

printModuleList();
