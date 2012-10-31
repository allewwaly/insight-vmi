
function handleModules(inst, members)
{
    println("Hier");
    println("Got called with instance \"" + inst.Name() + "\":");
    println(inst);
    for (var i = 0; i < members.length; ++i)
        println("member", i + ":", members[i], "(" + inst.MemberName(members[i]) + ")");
    println(members.toString());
//    inst;
    println("dort");
    return inst;
}
