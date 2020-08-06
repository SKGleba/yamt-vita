/*
	YAMT-V by SKGleba
	All Rights Reserved
*/

#include "tai_compat.h"
#include "defs.h"

static lite_cfg_struct cfg;
static SceIoDevice custom[2];
static int newfw = 0;

static const char *convc[4] = { "NULL", "sdstor0:ext-lp-act-entire", "sdstor0:xmc-lp-ign-userext", "sdstor0:int-lp-ign-userext" };

static int mthr(SceSize args, void *argp) {
	if (cfg.uxm > 0) {
		ksceIoUmount(0x800, 0, 0, 0);
		ksceIoUmount(0x800, 1, 0, 0);
		ksceIoMount(0x800, NULL, 0, 0, 0, 0);
	}
	if (cfg.umm > 0) {
		ksceIoUmount(0xF00, 0, 0, 0);
		ksceIoUmount(0xF00, 1, 0, 0);
		ksceIoMount(0xF00, NULL, 0, 0, 0, 0);
	}
	ksceKernelExitDeleteThread(0);
	return 1;
}

static void patch_uxuma(uint8_t uxm, uint8_t umm) {
	// Prepare iofilemgr patch stuff
	SceIoMountPoint *(* sceIoFindMountPoint)(int id) = NULL;
	size_t get_dev_mnfo_off = (newfw) ? 0x182f5 : 0x138c1;
	SceUID iof_modid = ksceKernelSearchModuleByName("SceIofilemgr");
	module_get_offset(KERNEL_PID, iof_modid, 0, get_dev_mnfo_off, (uintptr_t *)&sceIoFindMountPoint);
	
	// Patch
	SceIoMountPoint *mountp;
	if (uxm > 0) {
		mountp = sceIoFindMountPoint(0x800);
		custom[0].dev = mountp->dev->dev;
		custom[0].dev2 = mountp->dev->dev2;
		custom[0].blkdev = custom[1].blkdev2 = convc[uxm];
		custom[0].id = 0x800;
		mountp->dev = &custom[0];
	}
	
	if (umm > 0) {
		mountp = sceIoFindMountPoint(0xF00);
		custom[1].dev = mountp->dev->dev;
		custom[1].dev2 = mountp->dev->dev2;
		custom[1].blkdev = custom[1].blkdev2 = convc[umm];
		custom[1].id = 0xF00;
		mountp->dev = &custom[1];
	}
}

// GC-SD->ux0, MC->uma0
static void setupDefaultConfig(void) {
	cfg.ver = 0x34;
	cfg.ion = cfg.uxm = 0x31;
	cfg.umm = 0x32;
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args)
{
	// read cfg to mem
	memset(&cfg, 0, sizeof(cfg));
	int fd = ksceIoOpen("ur0:tai/yamt.cfg", SCE_O_RDONLY, 0);
	if (fd > 0) {
		ksceIoRead(fd, &cfg, sizeof(cfg));
		ksceIoClose(fd);
	}
	if (fd < 0 || cfg.ver != 0x34)
		setupDefaultConfig();
	
	newfw = tai_init();
		
	// Check isOn flag
	if (cfg.ion == 0x30 || newfw < 0)
		return SCE_KERNEL_START_SUCCESS;
		
	uint8_t uxm = cfg.uxm - 0x30;
	uint8_t umm = cfg.umm - 0x30;
	
	int ok = 1;
	if (uxm == 1 || umm == 1)
		ok = ((unsigned int)(*(int *)(*(int *)(ksceSdifGetSdContextGlobal(1) + 0x2430) + 0x24) << 0xf) >> 0x1f);
	
	// check config integrity
	if (uxm > 3 || umm > 3 || !ok)
		return SCE_KERNEL_START_SUCCESS;
	
	char movsr01[2] = {0x01, 0x20};
	INJECT("SceSysmem", 0x21610, movsr01, sizeof(movsr01));
	
	patch_uxuma(uxm, umm);
	
	// ksceIoMount threaded to save time
	ksceKernelStartThread(ksceKernelCreateThread("x", mthr, 0x00, 0x1000, 0, 0, 0), 0, NULL);
	
	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args)
{
	return SCE_KERNEL_STOP_SUCCESS;
}
