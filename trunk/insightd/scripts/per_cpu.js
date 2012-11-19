/*
Generic function to dereference a per-cpu member within a struct/union.

References:
http://lxr.linux.no/#linux+v2.6.32.58/include/linux/percpu-defs.h#L8
http://lxr.linux.no/#linux+v2.6.32.58/include/asm-generic/percpu.h#L18
http://lxr.linux.no/#linux+v2.6.32.58/include/asm-generic/percpu.h#L54
*/
function per_cpu_member(inst, members)
{
	for (var i in members) {
		inst = inst.Member(members[i]);
		if (!inst.IsValid() || inst.IsNull())
			return false;
	}
	return per_cpu_var(inst);
}

/*
Generic function to dereference a per-cpu variable.

References:
http://lxr.linux.no/#linux+v2.6.32.58/include/linux/percpu-defs.h#L8
http://lxr.linux.no/#linux+v2.6.32.58/include/asm-generic/percpu.h#L18
http://lxr.linux.no/#linux+v2.6.32.58/include/asm-generic/percpu.h#L54
*/
function per_cpu_var(inst)
{
	if (!inst.IsValid() || inst.IsNull())
		return false;
	var offsets = new Instance("__per_cpu_offset");
	// Is this an SMP system?
	if (offsets.IsValid()) {
		var offset = offsets.ArrayElem(0);
		inst.AddToAddress(offset.toULong(16));
	}
	return inst;
}