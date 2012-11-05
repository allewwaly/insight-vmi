
function showArgs(func, inst, members)
{
    println("Called " + func + "() with instance \"" + inst.Name() + "\":");
    for (var i = 0; i < members.length; ++i)
        println("    member[" + i + "] =", members[i], "\"" +
            inst.MemberName(members[i]) + "\"");
}


function getModules(inst, members)
{
//    showArgs("getModules", inst, members);
    // The next and prev members point to struct module objects
    inst = inst.Member(members[0]);
    inst.ChangeType("module");
    var offset = inst.MemberOffset("list");
    inst.AddToAddress(-offset);

    return inst;
}


function getModuleList(inst, members)
{
//    showArgs("getModuleList", inst, members);
    var typeId = inst.TypeId();
    inst = inst.Member(members[0]).Member(members[1]);
    // Compare to head of the list (which is no module)
    var m = new Instance("modules");
    if (m.Address() == inst.Address())
        return m;

    // Change type, fix offset
    inst.ChangeType(typeId);
    inst.AddToAddress(-inst.MemberOffset(members[0]));

    return inst;
}

