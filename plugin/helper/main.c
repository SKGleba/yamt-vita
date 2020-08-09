#include <stdio.h>
#include <string.h>
#include <psp2kern/kernel/modulemgr.h>
#include <vitasdkkern.h>
#include "../kernel/defs.h"
#define LOG_LOC "ur0:temp/yamt_helper.log"
#define LOGGING_ENABLED 0
#include "logging.h"
#include <taihen.h>

static int umaid = 0;
static uint8_t mids[16];
static char flags[4];

int yamtUserCmdHandler(int cmd, void *cmdbuf) {
	int state = 0;
	int opret = -1;
	uint8_t *cmids;
	if (cmdbuf == NULL)
		return opret;
	usercmd buf;
	ENTER_SYSCALL(state);
	ksceKernelMemcpyUserToKernel(&buf, (uintptr_t)cmdbuf, 0x210);
	EXIT_SYSCALL(state);
	if (buf.magic == 0xCAFEBABE) {
		switch (cmd) {
			case 0: // read SD to buf
				opret = ksceSdifReadSectorSd(ksceSdifGetSdContextPartValidateSd(1), buf.arg0, (void *)&buf.data, 1);
				break;
			case 1: // write buf to SD
				opret = ksceSdifWriteSectorSd(ksceSdifGetSdContextPartValidateSd(1), buf.arg0, &buf.data, 1);
				break;
			case 2: // remount everything
				cmids = (buf.arg0) ? &buf.data : &mids;
				for (int i = 0; i < 16; ++i) {
					if (cmids[i] > 0) {
						ksceIoUmount(cmids[i] * 0x100, 0, 0, 0);
						ksceIoUmount(cmids[i] * 0x100, 1, 0, 0);
						ksceIoMount(cmids[i] * 0x100, NULL, buf.arg1, 0, 0, 0);
					}
				}
				opret = 0;
				break;
			default:
				break;
		}
	}
	ENTER_SYSCALL(state);
	ksceKernelMemcpyKernelToUser((uintptr_t)cmdbuf, &buf, 0x210);
	EXIT_SYSCALL(state);
	return opret;
}

void patch_appmgr() {
	tai_module_info_t sceappmgr_modinfo;
	sceappmgr_modinfo.size = sizeof(tai_module_info_t);
	if (taiGetModuleInfoForKernel(KERNEL_PID, "SceAppMgr", &sceappmgr_modinfo) >= 0) {
		uint32_t nop_nop_opcode = 0xBF00BF00;
		taiInjectDataForKernel(KERNEL_PID, sceappmgr_modinfo.modid, 0, 0xB338, &nop_nop_opcode, 4);
		taiInjectDataForKernel(KERNEL_PID, sceappmgr_modinfo.modid, 0, 0xB368, &nop_nop_opcode, 2);
	}
}

void addUsbPatches(void) {
	int (* _ksceKernelMountBootfs)(const char *bootImagePath);
	int (* _ksceKernelUmountBootfs)(void);
	int ret;
	int (* yamtPatchUsbDrv)(const char *path) = NULL;
	module_get_export_func(KERNEL_PID, "yamtKernel", 0x664c9471, 0x8035aed3, (uintptr_t *)&yamtPatchUsbDrv);
	ret = module_get_export_func(KERNEL_PID, "SceKernelModulemgr", 0xC445FA63, 0x01360661, (uintptr_t *)&_ksceKernelMountBootfs);
	if (ret < 0)
		module_get_export_func(KERNEL_PID, "SceKernelModulemgr", 0x92C9FFC2, 0x185FF1BC, (uintptr_t *)&_ksceKernelMountBootfs);

	ret = module_get_export_func(KERNEL_PID, "SceKernelModulemgr", 0xC445FA63, 0x9C838A6B, (uintptr_t *)&_ksceKernelUmountBootfs);
	if (ret < 0)
		module_get_export_func(KERNEL_PID, "SceKernelModulemgr", 0x92C9FFC2, 0xBD61AD4D, (uintptr_t *)&_ksceKernelUmountBootfs);
	
	SceUID sceusbmass_modid;
	LOG("Mounting bootfs for umass... ");
	if (_ksceKernelMountBootfs("os0:kd/bootimage.skprx") >= 0) {
		ret = yamtPatchUsbDrv("os0:kd/umass.skprx");
		LOG("ok\npatch_usb: 0x%X\n", ret);
		_ksceKernelUmountBootfs();
	} else
		LOG("failed\n");
}

static int yusb_sysevent_handler(int resume, int eventid, void *args, void *opt) {
	if ((resume) && (eventid == 0x100000) && umaid > 0) {
		for (int i = 0; i < 26; i++) {
			if (ksceIoMount(umaid, NULL, 0, 0, 0, 0) == 0)
				break;
			else
				ksceKernelDelayThread(200000);
		}
	}
	return 0;
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args)
{
	// always clean mp ids
	memset(&mids, 0, 16);
	
	// check if yamt is running
	if (ksceKernelSearchModuleByName("yamtKernel") < 0) {
		LOG("yamt was NOT loaded\n");
		return SCE_KERNEL_START_SUCCESS;
	}
	
	// get yamt exports
	int (* yamtGPIO)(uint32_t mode, uint32_t slot, uint32_t value) = NULL;
	int (* yamtGetValidMids)(void* dst) = NULL;
	int (* yamtGetCDevID)(int master, int slave) = NULL;
	module_get_export_func(KERNEL_PID, "yamtKernel", 0x664c9471, 0xed6203f3, (uintptr_t *)&yamtGPIO);
	module_get_export_func(KERNEL_PID, "yamtKernel", 0x664c9471, 0x23d52276, (uintptr_t *)&yamtGetValidMids);
	module_get_export_func(KERNEL_PID, "yamtKernel", 0x664c9471, 0x9944d619, (uintptr_t *)&yamtGetCDevID);
	if (yamtGPIO == NULL || yamtGetCDevID == NULL || yamtGetValidMids == NULL) {
		LOG("could NOT get yamt exports (??)\n");
		return SCE_KERNEL_START_SUCCESS;
	}
	
	while(yamtGPIO(0, 2, 0) == 2) {
		LOG("yamt_usb busy, waiting 0.05s\n");
		ksceKernelDelayThread(50000);
	}
	
	// init log sys
	*(uint32_t *)flags = yamtGPIO(0, 0, 0);
	enable_logging = flags[1];
	LOG_START("yamt helper started %d, flags:\n use_usb %d | filog %d | usb_leg %d | nowake %d\n", yamtGPIO(0, 2, 0), flags[0], flags[1], flags[2], flags[3]);
	
	// usb stuff
	umaid = yamtGPIO(0, 3, 0);
	if (flags[0] && umaid > 0) {
		yamtGPIO(1, 2, 0);
		if (ksceKernelSearchModuleByName("SceUsbMass") >= 0) {
			int fd;
mounted: 
			fd = ksceIoOpen("sdstor0:uma-lp-act-entire", SCE_O_RDONLY, 0);
			if (fd < 0)
				fd = ksceIoOpen("sdstor0:uma-pp-act-a", SCE_O_RDONLY, 0);
			if (flags[2]) {
				LOG("forcing legacy usb mode\n");
				for (int i = 0; i <= 25; i++) {
					if (fd >= 0)
						break;
					LOG("checking %d\n", i);
					fd = ksceIoOpen("sdstor0:uma-lp-act-entire", SCE_O_RDONLY, 0);
					if (fd >= 0)
						break;
					fd = ksceIoOpen("sdstor0:uma-pp-act-a", SCE_O_RDONLY, 0);
					if (fd >= 0)
						break;
					ksceKernelDelayThread(200000);
				}
				if (fd >= 0) {
					ksceIoClose(fd);
					if (umaid == 0x800)
						patch_appmgr();
					ksceIoUmount(umaid, 0, 0, 0);
					ksceIoUmount(umaid, 1, 0, 0);
					LOG("found uma, legacy redir 0x%X\n", ksceIoMount(umaid, NULL, 0, 0, 0, 0));
				}
			} else {
				if (fd >= 0) {
					ksceIoClose(fd);
					if (umaid == 0x800)
						patch_appmgr();
					LOG("found uma, redir 0x%X\n", ksceIoMount(umaid, NULL, 0, 0, 0, 0));
				}	
			}
			LOG("umass module loaded, uma fd = 0x%X, umaid = 0x%X\n", fd, umaid);
			if (fd >= 0 && (!flags[3]))
				ksceKernelRegisterSysEventHandler("yusb_sysevent", yusb_sysevent_handler, NULL);
		} else {
			addUsbPatches();
			goto mounted;
		}
	}
	
	// mount if yamt didnt make it
	yamtGPIO(1, 1, 0);
	yamtGetValidMids(&mids);
	if (!flags[3]) {
		for (int i = 0; i < 16; ++i) {
			if (mids[i] > 0)
				LOG("mounting 0x%X = 0x%X\n", mids[i] * 0x100, ksceIoMount(mids[i] * 0x100, NULL, 0, 0, 0, 0));
		}
	}
	
	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args)
{
	return SCE_KERNEL_STOP_SUCCESS;
}
