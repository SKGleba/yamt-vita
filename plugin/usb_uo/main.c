#include <stdio.h>
#include <string.h>
#include <psp2kern/kernel/modulemgr.h>

#define LOG_LOC "ur0:temp/usbuo.log"
#define LOGGING_ENABLED 0
#include "logging.h"

typedef struct {
	const char *dev;
	const char *dev2;
	const char *blkdev;
	const char *blkdev2;
	int id;
} SceIoDevice;

static int id = 0, gusb = 0;
const char check_patch[] = {0x01, 0x20, 0x01, 0x20};
const char stub_ret_one[] = {0x01, 0x00, 0xa0, 0xe3, 0x1e, 0xff, 0x2f, 0xe1};
static SceIoDevice uma_ux0_dev = { "ux0:", "exfatux0", "sdstor0:uma-pp-act-a", "sdstor0:uma-lp-act-entire", 0x800 };

static int yusb_sysevent_handler(int resume, int eventid, void *args, void *opt) {
	if ((resume) && (eventid == 0x100000) && (gusb)) {
		for (int i = 0; i < 26; i++) {
			if (ksceIoMount(id, NULL, 0, 0, 0, 0) == 0)
				break;
			else
				ksceKernelDelayThread(200000);
		}
	}
	return 0;
}

static void mount_old(void) {
	int fd, ret;
	for (int i = 0; i < 26; i++) {
		// try to detect USB plugin 25 times for 0.2s each
		fd = ksceIoOpen("sdstor0:uma-lp-act-entire", SCE_O_RDONLY, 0);
		if (fd >= 0) {
			ksceIoClose(fd);
			ret = yamtMount(uma_ux0_dev, 0x800);
			if (ret == 0)
				gusb = 1;
			LOG("mount_old 0x%X\n", ret);
			return;
			break;
		}
		ksceKernelDelayThread(200000);
	}
	LOG("mount_old failed\n");
	return;
}

static int mthr(SceSize args, void *argp) {
	if (id == 0)
		return -1;
	while (ksceKernelSysrootGetShellPid() < 0) {
		if (yamtMount(NULL, id) == 0) {
			LOG("mounted\n");
			gusb = 1;
			ksceKernelExitDeleteThread(0);
			break;
		} else
			ksceKernelDelayThread(200000);
	}
	LOG("task failed successfully\n");
	ksceKernelExitDeleteThread(0);
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args)
{
	// check [enable] status
	if (yamtCheckGPO(0) < 1)
		return SCE_KERNEL_START_SUCCESS;
	// get USB id
	id = yamtGetCDevID(5, 16); // uma---entire
	if (id == 0)
		id = yamtGetCDevID(5, 0); // uma---a
	enable_logging = yamtCheckGPO(1);
	LOG_START("usbuo started with id 0x%X\n", id);
	if (id == 0)
		return SCE_KERNEL_START_SUCCESS;
		
	// patch usbserv's safemode syscall
	yamtInject("SceUsbServ", 0x22ec, (void *)stub_ret_one, sizeof(stub_ret_one));
		
	SceUID modid = ksceKernelLoadModule("os0:kd/umass.skprx", 0x800, NULL);
	if (modid < 0)
		return SCE_KERNEL_START_FAILED;
		
	LOG("loaded umass w id 0x%X\n", modid);
	
	yamtInject("SceUsbMass", 0x1546, (void *)check_patch, sizeof(check_patch));
	yamtInject("SceUsbMass", 0x154c, (void *)check_patch, sizeof(check_patch));
	
	if (ksceKernelStartModule(modid, 0, NULL, 0, NULL, NULL) < 0) {
		ksceKernelUnloadModule(modid, 0, NULL);
		return SCE_KERNEL_START_FAILED;
	}	
	
	// Add sehandler for umass mounting at resume
	if (!yamtCheckGPO(3))
		ksceKernelRegisterSysEventHandler("yusb_sysevent", yusb_sysevent_handler, NULL);
	
	LOG("patched & SEHed, remounting\n");
	
	// mount the usb
	if (yamtCheckGPO(2))
		mount_old();
	else
		ksceKernelStartThread(ksceKernelCreateThread("x", mthr, 0x00, 0x1000, 0, 0, 0), 0, NULL);
	
	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args)
{
	return SCE_KERNEL_STOP_SUCCESS;
}
