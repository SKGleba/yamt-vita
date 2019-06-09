#include "tai_compat.h"

static int mthr(SceSize args, void *argp) {
	ksceIoUmount(0x800, 0, 0, 0);
	ksceIoUmount(0x800, 1, 0, 0);
	ksceIoMount(0x800, NULL, 0, 0, 0, 0);
	ksceIoMount(0xF00, NULL, 0, 0, 0, 0);
	ksceKernelExitDeleteThread(0);
	return 1;
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args)
{
	// Check if theres something in the gc slot
	if (((unsigned int)(*(int *)(*(int *)(ksceSdifGetSdContextGlobal(1) + 0x2430) + 0x24) << 0xf) >> 0x1f) < 1)
		return SCE_KERNEL_START_NO_RESIDENT;
	
	// Patch sdstor SD checks
	char movsr01[2] = {0x01, 0x20};
	INJECT("SceSysmem", 0x21610, movsr01, sizeof(movsr01));
	
	// String edit iof
	size_t base_oof = (0x1D340 - 0xA0);
#ifdef FW365
	base_oof = (0x1D498 - 0xA0);
#endif
	static char sd0_safe_blkn[] = "sdstor0:gcd-lp-act-entire"; // pseudo-sd0 blkn (for compat)
	static char xmc0_blkn[] = "sdstor0:xmc-lp-ign-userext";
	static char imc0_blkn[] = "sdstor0:int-lp-ign-userext";
	INJECT("SceIofilemgr", (base_oof + 0x808), sd0_safe_blkn, sizeof(sd0_safe_blkn));
	INJECT("SceIofilemgr", (base_oof + 0x43C), (kscePervasiveRemovableMemoryGetCardInsertState()) ? xmc0_blkn : imc0_blkn, sizeof(xmc0_blkn));
	
	// ksceIoMount threaded to save time
	SceUID mthid = ksceKernelCreateThread("x", mthr, 0x00, 0x1000, 0, 0, 0);
	ksceKernelStartThread(mthid, 0, NULL);
	
	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args)
{
	return SCE_KERNEL_STOP_SUCCESS;
}
