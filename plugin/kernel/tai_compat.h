#include <stdio.h>
#include <string.h>
#include <taihen.h>
#include <psp2kern/kernel/modulemgr.h>
#include <vitasdkkern.h>
	
#define INJECT(name, off, data, sz)   \
do {                                \
	uintptr_t addr;					\
	tai_module_info_t info;			\
	info.size = sizeof(info);		\
	if (module_get_by_name_nid(KERNEL_PID, name, &info) >= 0) {	\
		module_get_offset(KERNEL_PID, info.modid, 0, off, &addr); \
		ksceKernelCpuUnrestrictedMemcpy((void *)addr, (void *)data, sz);	\
	}									\
} while (0)
	
#define MOD_LIST_SIZE (256)
