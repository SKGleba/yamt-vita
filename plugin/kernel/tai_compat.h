#include <stdio.h>
#include <string.h>
#include <taihen.h>
#include <psp2kern/kernel/modulemgr.h>
#include <vitasdkkern.h>
	
#define NOP_MOD_RANGE(name, off, bc)   \
do {                                \
	uintptr_t addr;					\
	tai_module_info_t info;			\
	info.size = sizeof(info);		\
	if (module_get_by_name_nid(KERNEL_PID, name, &info) >= 0) {	\
		module_get_offset(KERNEL_PID, info.modid, 0, off, &addr); \
		int curr = 0;					\
		uint16_t nop_opcode = 0xBF00;	\
		while (curr < bc) {				\
			ksceKernelCpuUnrestrictedMemcpy((void *)(addr + curr), &nop_opcode, sizeof(nop_opcode));	\
			curr = curr + 2;			\
		}								\
	}									\
} while (0)
	
#define INJECT_CCACHE(name, off, data, sz)   \
do {                                \
	uintptr_t addr;					\
	tai_module_info_t info;			\
	info.size = sizeof(info);		\
	if (module_get_by_name_nid(KERNEL_PID, name, &info) >= 0) {	\
		module_get_offset(KERNEL_PID, info.modid, 0, off, &addr); \
		ksceKernelCpuUnrestrictedMemcpy((void *)addr, (void *)data, sz);	\
		ksceKernelCpuDcacheAndL2WritebackInvalidateRange((void *)addr, sz); \
	}									\
} while (0)

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
	
#define INJECT_NOGET(info, off, data, sz)   \
do {                                \
	uintptr_t addr;					\
	module_get_offset(KERNEL_PID, info.modid, 0, off, &addr); \
	ksceKernelCpuUnrestrictedMemcpy((void *)addr, (void *)data, sz);	\
} while (0)
	
#define MOD_LIST_SIZE (256)
