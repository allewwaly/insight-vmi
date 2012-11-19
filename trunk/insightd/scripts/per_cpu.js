/*
Generic function to dereference a per-cpu member within a struct/union.
It is assumed that only one member is accessed!

References:
http://lxr.linux.no/#linux+v2.6.32.58/include/linux/percpu-defs.h#L8
http://lxr.linux.no/#linux+v2.6.32.58/include/asm-generic/percpu.h#L18
http://lxr.linux.no/#linux+v2.6.32.58/include/asm-generic/percpu.h#L54
*/
function per_cpu_member(inst, members)
{
	inst = inst.Member(members[0]);
	var offsets = new Instance("__per_cpu_offset");
	// Is this an SMP system?
	if (offsets.IsValid()) {
		var offset = offsets.ArrayElem(0);
		inst.AddToAddress(offset.toULong(16));
	}
	return inst;
}