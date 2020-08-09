/*
	YAMT-V by SKGleba
	All Rights Reserved
*/

#include "tai_compat.h"
#include "defs.h"

static cfg_struct cfg;
static int newfw = 0, umaid = 0;
static uint8_t mids[16];
static char convc[16][64];
static SceIoDevice custom[16];
static uint32_t gpio[4];
const char umass_skipb[] = {0x0d, 0xe0, 0x00, 0xbf};
const char stub_ret_one[] = {0x01, 0x00, 0xa0, 0xe3, 0x1e, 0xff, 0x2f, 0xe1};
static const char *psvsdpdev = "sdstor0:uma-pp-act-a";

int yamtPatchUsbDrv(const char *drvpath) {
	gpio[2] = 2;
	INJECT("SceUsbServ", 0x22ec, &stub_ret_one, sizeof(stub_ret_one));
	SceUID modid = ksceKernelLoadModule(drvpath, 0x800, NULL);
	if (modid >= 0) {
		INJECT_NOGET(modid, 0x1546, &umass_skipb, sizeof(umass_skipb));
		ksceKernelStartModule(modid, 0, NULL, 0, NULL, NULL);
		gpio[2] = 0;
		if (umaid > 0)
			ksceIoMount(umaid, NULL, 0, 0, 0, 0);
	}
	gpio[2] = 0;
	return modid;
}

static int mthr(SceSize args, void *argp) {
	gpio[1] = 1;
	int curid = 0;
	while (curid < 15 && (gpio[1])) {
		if (mids[curid] > 0) {
			ksceIoUmount(mids[curid] * 0x100, 0, 0, 0);
			ksceIoUmount(mids[curid] * 0x100, 1, 0, 0);
			ksceIoMount(mids[curid] * 0x100, NULL, 0, 0, 0, 0);
		}
		curid-=-1;
	}
	gpio[1] = 0;
	if (cfg.drv[0] && !cfg.drv[3]) {
		gpio[2] = 1;
		ksceKernelDelayThread(500000);
		if (gpio[2] == 1 && ksceKernelSearchModuleByName("SceUsbServ") >= 0)
			yamtPatchUsbDrv("os0:kd/umass.skprx");
		gpio[2] = 0;
	}
	ksceKernelExitDeleteThread(0);
	return 1;
}

static int req(uint8_t req, uint8_t wstat) {
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

static void patch_redirect(void) {
	size_t base_oof = (newfw) ? (0x1D498 - 0xA0) : (0x1D340 - 0xA0);
	size_t get_dev_mnfo_off = (newfw) ? 0x182f5 : 0x138c1;
	SceIoMountPoint *(* sceIoFindMountPoint)(int id) = NULL;
	SceUID iof_modid = ksceKernelSearchModuleByName("SceIofilemgr");
	module_get_offset(KERNEL_PID, iof_modid, 0, get_dev_mnfo_off, (uintptr_t *)&sceIoFindMountPoint);
	if (iof_modid < 0 || sceIoFindMountPoint == NULL)
		return;
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
				custom[vetr].blkdev = (umaid == cfg.entry[loopos].mid * 0x100) ? psvsdpdev : convc[vetr]; // add physdev support for psvsd compat
				custom[vetr].blkdev2 = convc[vetr];
				custom[vetr].id = (cfg.entry[loopos].mid * 0x100);
				mount1->dev = &custom[vetr];
				mount1->dev2 = &custom[vetr];
			} else if (cfg.entry[loopos].mode == 0 && cfg.entry[loopos].t_off != 0xDEADBEEF)
				INJECT_NOGET(iof_modid, (base_oof + cfg.entry[loopos].t_off), convc[vetr], strlen(convc[vetr]) + 1);
			mids[vetr] = cfg.entry[loopos].mid;
			vetr = vetr + 1;
		}
		loopos = loopos + 1;
	}
}

int yamtGetCDevID(int master, int slave) {
	if (cfg.ion == 0)
		return 0;
	for (int i = 0; i < 16; ++i) {
		if ((cfg.entry[i].devn[0] == master) && (cfg.entry[i].devn[3] == slave))
			return (cfg.entry[i].mid * 0x100);
	}
	return 0;
}

int yamtGetValidMids(void* dst) {
	if (cfg.ion == 0)
		return -1;
	memcpy(dst, &mids, 16);
	return 0;
}

int yamtGPIO(uint32_t mode, uint32_t slot, uint32_t value) {
	if (mode > 1 || slot > 3)
		return 0;
	if (mode)
		gpio[slot] = value;
	return gpio[slot];
}

// GC-SD->ux0, MC->uma0
static void setupDefaultConfig(void) {
	cfg.ion = cfg.mrq = 1;
	cfg.vec = 0;
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args)
{
	// read cfg to mem
	memset(&gpio, 0, sizeof(gpio));
	memset(&cfg, 0, sizeof(cfg));
	int fd = ksceIoOpen("ur0:tai/yamt.cfg", SCE_O_RDONLY, 0);
	if (fd > 0) {
		ksceIoRead(fd, &cfg, sizeof(cfg));
		ksceIoClose(fd);
	}
	if (fd < 0 || cfg.ver != 4)
		setupDefaultConfig();
	
	newfw = tai_init();
	
	// Check config flags
	if (cfg.ion == 0 || cfg.vec > 16 || newfw < 0)
		return SCE_KERNEL_START_SUCCESS;
	
	// Patch SD checks
	if (cfg.mrq != 0) {	
		char movsr01[2] = {0x01, 0x20};
		INJECT("SceSysmem", 0x21610, movsr01, sizeof(movsr01));
	}
	
	gpio[0] = *(uint32_t *)cfg.drv;
	
	umaid = yamtGetCDevID(5, 16); // uma---entire
	if (umaid == 0)
		umaid = yamtGetCDevID(5, 0); // uma---a
	
	// "Big" redirect loop
	patch_redirect();
	
	// ksceIoMount threaded to save time
	ksceKernelStartThread(ksceKernelCreateThread("x", mthr, 0x00, 0x1000, 0, 0, 0), 0, NULL);
	
	gpio[3] = umaid;
	
	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args)
{
	return SCE_KERNEL_STOP_SUCCESS;
}
