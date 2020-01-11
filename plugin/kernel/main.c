/*
	YAMT-V by SKGleba
	All Rights Reserved
*/

#include "tai_compat.h"
#include "defs.h"

static cfg_struct cfg;
static uint8_t mids[16];
static char convc[16][64];
static SceIoDevice custom[16];
static SceIoMountPoint *(* sceIoFindMountPoint)(int id) = NULL;

static int mthr(SceSize args, void *argp) {
	int curid = 0;
	while (curid < 15) {
		if (mids[curid] > 0) {
			ksceIoUmount(mids[curid] * 0x100, 0, 0, 0);
			ksceIoUmount(mids[curid] * 0x100, 1, 0, 0);
			ksceIoMount(mids[curid] * 0x100, NULL, 0, 0, 0, 0);
		}
		curid = curid + 1;
	}
	ksceKernelExitDeleteThread(0);
	return 1;
}

int req(uint8_t req, uint8_t wstat) {
	if (req == 0)
		return 1;
	int ret = 0;
	if (req == 1) {
		ret = ((unsigned int)(*(int *)(*(int *)(ksceSdifGetSdContextGlobal(1) + 0x2430) + 0x24) << 0xf) >> 0x1f) > 0 ? 1 : 0; // check insert state for gc slot
	} else if (req == 2) {
		ret = kscePervasiveRemovableMemoryGetCardInsertState() ? 1 : 0;
	} else if (req == 3) {
		ret = (*(uint32_t *)(*(int *)(ksceKernelGetSysbase() + 0x6c) + 0xD8) & 0x8) != 0 ? 1 : 0; // check AC state in sysroot->kbl_param+0xD8
	}
	if (ret == wstat)
		return 1;
	return 0;		
}

void patch_redirect(void) {
	size_t get_dev_mnfo_off = 0x138c1;
	size_t base_oof = (0x1D340 - 0xA0);
#ifdef FW365
	get_dev_mnfo_off = 0x182f5;
	base_oof = (0x1D498 - 0xA0);
#endif
	tai_module_info_t finfo;
	finfo.size = sizeof(finfo);
	module_get_by_name_nid(KERNEL_PID, "SceIofilemgr", &finfo);
	module_get_offset(KERNEL_PID, finfo.modid, 0, get_dev_mnfo_off, (uintptr_t *)&sceIoFindMountPoint);
	SceIoMountPoint *mount1;
	int loopos = 0, vetr = 0;
	while (loopos < cfg.vec) {
		if (cfg.entry[loopos].devn[0] != 0xFF && req(cfg.entry[loopos].reqs[0], cfg.entry[loopos].reqs[1])) {
			memset(convc[vetr], 0, 64);
			snprintf(convc[vetr], sizeof(convc[vetr]), "sdstor0:%s-lp-%s-%s", st[cfg.entry[loopos].devn[0]], rd[cfg.entry[loopos].devn[2]], th[cfg.entry[loopos].devn[3]]);
			if (cfg.entry[loopos].mode == 1) {
				mount1 = sceIoFindMountPoint(cfg.entry[loopos].mid * 0x100);
				custom[vetr].dev = mount1->dev->dev;
				custom[vetr].dev2 = mount1->dev->dev2;
				custom[vetr].blkdev = custom[vetr].blkdev2 = convc[vetr];
				custom[vetr].id = (cfg.entry[loopos].mid * 0x100);
				mount1->dev = &custom[vetr];
			} else if (cfg.entry[loopos].mode == 0 && cfg.entry[loopos].t_off != 0xDEADBEEF)
				INJECT_NOGET(finfo, (base_oof + cfg.entry[loopos].t_off), convc[vetr], strlen(convc[vetr]) + 1);
			mids[vetr] = cfg.entry[loopos].mid;
			vetr = vetr + 1;
		}
		loopos = loopos + 1;
	}
}

// GC-SD->ux0, MC->uma0
void setupDefaultConfig(void) {
	cfg.ion = cfg.mrq = cfg.entry[0].reqs[0] = cfg.entry[1].reqs[0] = cfg.entry[0].mode = 1;
	cfg.entry[1].mode = cfg.entry[0].reqs[1] = cfg.entry[1].reqs[1] = cfg.entry[0].devn[2] = 1;
	cfg.vec = cfg.entry[0].devn[0] = cfg.entry[1].devn[2] = 2;
	cfg.entry[0].devn[1] = cfg.entry[1].devn[1] = 0;
	cfg.ver = 3;
	cfg.entry[0].devn[3] = 16;
	cfg.entry[0].mid = 0x8;
	cfg.entry[1].devn[0] = 4;
	cfg.entry[1].devn[3] = 9;
	cfg.entry[1].mid = 0xF;
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
	if (fd < 0 || cfg.ver != 3)
		setupDefaultConfig();
	
	// Check config flags
	if (cfg.ion == 0 || cfg.vec > 16)
		return SCE_KERNEL_START_NO_RESIDENT;
	
	// Patch SD checks
	if (cfg.mrq != 0) {	
		char movsr01[2] = {0x01, 0x20};
		INJECT("SceSysmem", 0x21610, movsr01, sizeof(movsr01));
	}
	
	// "Big" redirect loop
	patch_redirect();
	
	// ksceIoMount threaded to save time
	SceUID mthid = ksceKernelCreateThread("x", mthr, 0x00, 0x1000, 0, 0, 0);
	ksceKernelStartThread(mthid, 0, NULL);
	
	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args)
{
	return SCE_KERNEL_STOP_SUCCESS;
}
