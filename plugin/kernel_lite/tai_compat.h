#include <stdio.h>
#include <string.h>
#include <taihen.h>
#include <psp2kern/kernel/modulemgr.h>
#include <vitasdkkern.h>

#define DACR_OFF(stmt)                 \
do {                                   \
    unsigned prev_dacr;                \
    __asm__ volatile(                  \
        "mrc p15, 0, %0, c3, c0, 0 \n" \
        : "=r" (prev_dacr)             \
    );                                 \
    __asm__ volatile(                  \
        "mcr p15, 0, %0, c3, c0, 0 \n" \
        : : "r" (0xFFFF0000)           \
    );                                 \
    stmt;                              \
    __asm__ volatile(                  \
        "mcr p15, 0, %0, c3, c0, 0 \n" \
        : : "r" (prev_dacr)            \
    );                                 \
} while (0)
	
#define NOP_MOD_RANGE(name, off, bc)   \
do {                                \
	uintptr_t addr;					\
	int modid = ksceKernelSearchModuleByName(name); \
	if (modid >= 0) {	\
		module_get_offset(KERNEL_PID, modid, 0, off, &addr); \
		int curr = 0;					\
		uint16_t nop_opcode = 0xBF00;	\
		while (curr < bc) {				\
			DACR_OFF(memcpy((void *)(addr + curr), &nop_opcode, sizeof(nop_opcode)););	\
			curr = curr + 2;			\
		}								\
	}									\
} while (0)

#define INJECT(name, off, data, sz)   \
do {                                \
	uintptr_t addr;					\
	int modid = ksceKernelSearchModuleByName(name); \
	if (modid >= 0) {	\
		module_get_offset(KERNEL_PID, modid, 0, off, &addr); \
		DACR_OFF(memcpy((void *)addr, (void *)data, sz););	\
	}									\
} while (0)
	
#define INJECT_NOGET(modid, off, data, sz)   \
do {                                \
	uintptr_t addr;					\
	module_get_offset(KERNEL_PID, modid, 0, off, &addr); \
	DACR_OFF(memcpy((void *)addr, (void *)data, sz););	\
} while (0)
	
#define MOD_LIST_SIZE (256)
