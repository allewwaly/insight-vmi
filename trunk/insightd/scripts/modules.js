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
		m = m.list.next;
	} while (m && m.Address() != head.Address());
}

printModuleList();