
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mm.h>

// cleanup some symbols
#undef __always_inline

#include <stdio.h>

// As long as there is no VMEMMAP_END defined, we have to define
// it ourselves (see Documentation/x86_64/mm.txt)
#ifndef VMEMMAP_END

#	define VMEMMAP_BITS 40
#	define VMEMMAP_END (VMEMMAP_START | ((1UL << VMEMMAP_BITS)-1))

#endif

int init_module(void)
{
	return 0;
}
 
void cleanup_module(void)
{
}

int main()
{
	printf("%-16s = 0x%16lx\n", "START_KERNEL_map", __START_KERNEL_map);
	printf("%-16s = 0x%16lx\n", "VMALLOC_START", VMALLOC_START);
	printf("%-16s = 0x%16lx\n", "VMALLOC_END", VMALLOC_END);
	printf("%-16s = 0x%16lx\n", "PAGE_OFFSET", __PAGE_OFFSET);
	printf("%-16s = 0x%16lx\n", "MODULES_VADDR", MODULES_VADDR);
	printf("%-16s = 0x%16lx\n", "MODULES_END", MODULES_END);
	printf("%-16s = 0x%16lx\n", "VMEMMAP_START", VMEMMAP_START);
	printf("%-16s = 0x%16lx\n", "VMEMMAP_END", VMEMMAP_END);
	printf("%-16s = %d\n", "SIZEOF_UNSIGNED_LONG", sizeof(unsigned long));

	return 0;
}

