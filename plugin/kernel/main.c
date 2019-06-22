/*
	YAMT-V by SKGleba
	All Rights Reserved
*/

#include "tai_compat.h"
#include "defs.h"

static uint8_t mids[16];

static int mthr(SceSize args, void *argp) {
	int curid = 0;
	while (mids[curid] > 0 && curid < 16) {
		ksceIoUmount(mids[curid] * 0x100, 0, 0, 0);
		ksceIoUmount(mids[curid] * 0x100, 1, 0, 0);
		ksceIoMount(mids[curid] * 0x100, NULL, 0, 0, 0, 0);
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

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args)
{
	// read cfg to mem
	static cfg_struct cfg;
	memset(&cfg, 0, sizeof(cfg));
	int fd = ksceIoOpen("ur0:tai/yamt.cfg", SCE_O_RDONLY, 0);
	if (fd < 0)
		return SCE_KERNEL_START_NO_RESIDENT;
	ksceIoRead(fd, &cfg, sizeof(cfg));
	ksceIoClose(fd);
	if (cfg.ion == 0 || cfg.ver != 2 || cfg.vec > 16)
		return SCE_KERNEL_START_NO_RESIDENT;
	
	if (cfg.mrq != 0) {	// Patch SD checks
		char movsr01[2] = {0x01, 0x20};
		INJECT("SceSysmem", 0x21610, movsr01, sizeof(movsr01));
	}
	
	// String edit iof
	size_t base_oof = (0x1D340 - 0xA0);
#ifdef FW365
	base_oof = (0x1D498 - 0xA0);
#endif
	int loopos = 0, vetr = 0;
	static char conve[64];
	while (loopos < cfg.vec) {
		if (cfg.entry[loopos].devn[0] != 0xFF && req(cfg.entry[loopos].reqs[0], cfg.entry[loopos].reqs[1])) {
			memset(&conve, 0, sizeof(conve));
			snprintf(conve, sizeof(conve), "sdstor0:%s-lp-%s-%s", st[cfg.entry[loopos].devn[0]], rd[cfg.entry[loopos].devn[2]], th[cfg.entry[loopos].devn[3]]);
			INJECT("SceIofilemgr", (base_oof + cfg.entry[loopos].t_off), conve, strlen(conve) + 1);
			mids[vetr] = cfg.entry[loopos].mid;
			vetr = vetr + 1;
		}
		loopos = loopos + 1;
	}
	
	// ksceIoMount threaded to save time
	SceUID mthid = ksceKernelCreateThread("x", mthr, 0x00, 0x1000, 0, 0, 0);
	ksceKernelStartThread(mthid, 0, NULL);
	
	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args)
{
	return SCE_KERNEL_STOP_SUCCESS;
}
