
function showArgs(func, inst, members)
{
    println("Called " + func + "() with instance \"" + inst.Name() + "\":");
    for (var i = 0; i < members.length; ++i)
        println("    member[" + i + "] =", members[i], "\"" +
            inst.MemberName(members[i]) + "\"");
}


function getModules(inst, members)
{
    showArgs("getModules", inst, members);
    // The next and prev members point to struct module objects
    inst = inst.Member(members[0]);
    inst.ChangeType("module");
    var offset = inst.MemberOffset("list");
    inst.AddToAddress(-offset);

    return inst;
}


function getModuleList(inst, members)
{
    showArgs("getModuleList", inst, members);

//    inst = inst.Member(members[0], true);
//    inst.ChangeType("module");
//    var offset = inst.MemberOffset("list");
//    inst.AddToAddress(-offset);

    return false;
}

