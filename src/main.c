#include <psp2/kernel/processmgr.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/devctl.h>
#include <psp2/sysmodule.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/io/stat.h>
#include <psp2/ctrl.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <psp2/kernel/modulemgr.h> 

#include "graphics.h"

#define printf psvDebugScreenPrintf

static unsigned buttons[] = {
	SCE_CTRL_SELECT,
	SCE_CTRL_START,
	SCE_CTRL_UP,
	SCE_CTRL_RIGHT,
	SCE_CTRL_DOWN,
	SCE_CTRL_LEFT,
	SCE_CTRL_LTRIGGER,
	SCE_CTRL_RTRIGGER,
	SCE_CTRL_TRIANGLE,
	SCE_CTRL_CIRCLE,
	SCE_CTRL_CROSS,
	SCE_CTRL_SQUARE,
};

int fap(const char *from, const char *to) {
	long psz;
	FILE *fp = fopen(from,"rb");

	fseek(fp, 0, SEEK_END);
	psz = ftell(fp);
	rewind(fp);

	char* pbf = (char*) malloc(sizeof(char) * psz);
	fread(pbf, sizeof(char), (size_t)psz, fp);

	FILE *pFile = fopen(to, "ab");
	
	for (int i = 0; i < psz; ++i) {
			fputc(pbf[i], pFile);
	}
   
	fclose(fp);
	fclose(pFile);
	return 1;
}

int fcp(const char *from, const char *to) {
	SceUID fd = sceIoOpen(to, SCE_O_WRONLY | SCE_O_TRUNC | SCE_O_CREAT, 6);
	sceIoClose(fd);
	int ret = fap(from, to);
	return ret;
}

int ex(const char *fname) {
    FILE *file;
    if ((file = fopen(fname, "r")))
    {
        fclose(file);
        return 1;
    }
    return 0;
}

uint32_t get_key(void) {
	static unsigned prev = 0;
	SceCtrlData pad;
	while (1) {
		memset(&pad, 0, sizeof(pad));
		sceCtrlPeekBufferPositive(0, &pad, 1);
		unsigned new = prev ^ (pad.buttons & prev);
		prev = pad.buttons;
		for (size_t i = 0; i < sizeof(buttons)/sizeof(*buttons); ++i)
			if (new & buttons[i])
				return buttons[i];

		sceKernelDelayThread(1000); // 1ms
	}
}

int installYamtB() {
	long psz;
	FILE *fp = fopen("ur0:tai/boot_config.txt", "rb");
	fseek(fp, 0, SEEK_END);
	psz = ftell(fp);
	rewind(fp);
	char* pbf = (char*) malloc(sizeof(char) * psz);
	fread(pbf, sizeof(char), (size_t)psz, fp);
	fclose(fp);
	sceIoRename("ur0:tai/boot_config.txt", "ur0:tai/boot_config.txt_oy");
	
	FILE *pFile = fopen("ur0:tai/boot_config.txt", "wb");
	char *pos = strstr(pbf, "\nload	os0:kd/hid.skprx");
	if (!pos) {
		printf("failed to patch config: cannot locate hid line\n");
		return -1;
	}
	char *pkx = strstr(pbf, "# YAMT\n");
	if (!pkx) {
		const char *patch1 = 
			"# YAMT\n- load\tur0:tai/yamt.skprx\n\n";
		fwrite(patch1, 1, strlen(patch1), pFile);
	}
	int len = pos - pbf;
	fwrite(pbf, 1, len, pFile);
	char *psx = strstr(pbf, "\n- load\tur0:tai/yamt_usb.skprx");
	if (!psx) {
		const char *patch2 = 
			"\n- load\tur0:tai/yamt_usb.skprx\n";
		fwrite(patch2, 1, strlen(patch2), pFile);
	}
	len = strlen(pos);
	fwrite(pos, 1, len, pFile);
	fclose(pFile);
	free(pbf);
	return 0;
}

void installYamtC() {
	long psz;
	FILE *fp = fopen("ur0:tai/config.txt", "rb");
	fseek(fp, 0, SEEK_END);
	psz = ftell(fp);
	rewind(fp);
	char* pbf = (char*) malloc(sizeof(char) * psz);
	fread(pbf, sizeof(char), (size_t)psz, fp);
	fclose(fp);
	sceIoRename("ur0:tai/config.txt", "ur0:tai/config.txt_oy");
	
	FILE *pFile = fopen("ur0:tai/config.txt", "wb");
	char *pkx = strstr(pbf, "# YAMT\n");
	if (!pkx) {
		const char *patch1 = 
			"# YAMT\n*NPXS10015\nur0:tai/yamt.suprx\n\n";
		fwrite(patch1, 1, strlen(patch1), pFile);
	}
	
	fwrite(pbf, 1, psz, pFile);
	fclose(pFile);
	free(pbf);
}

int main(int argc, char *argv[]) {
	(void)argc;
	(void)argv;

	psvDebugScreenInit();
	psvDebugScreenClear(COLOR_BLACK);
	
	static char version[16];
	
	SceKernelFwInfo data;
	data.size = sizeof(SceKernelFwInfo);
	_vshSblGetSystemSwVersion(&data);
	
	psvDebugScreenSetFgColor(COLOR_CYAN);
	printf("YAMT installer v4.0b3 by SKGleba\n\n");
	
	if (ex("ur0:tai/boot_config.txt") == 0) {
		psvDebugScreenSetFgColor(COLOR_RED);
		printf("Could not find boot_config.txt in ur0:tai/ !\n");
		sceKernelDelayThread(3 * 1000 * 1000);
		sceKernelExitProcess(0);
		sceKernelDelayThread(3 * 1000 * 1000);
	}
	
	psvDebugScreenSetFgColor(COLOR_WHITE);
	if (ex("ur0:tai/yamt.skprx")) {
		printf("removing the YAMT driver... ");
		sceIoRemove("ur0:tai/yamt.skprx");
		sceIoRemove("ur0:tai/yamt.suprx");
		printf("ok!\n");
		psvDebugScreenSetFgColor(COLOR_YELLOW);
		printf("\npress CROSS to remove the storage config\nor press any other key to continue... ");
		if (get_key() == SCE_CTRL_CROSS)
			sceIoRemove("ur0:tai/yamt.cfg");
		psvDebugScreenSetFgColor(COLOR_WHITE);
		printf("ok!\nremoving the USB compat driver... ");
		sceIoRemove("ur0:tai/yamt_usb.skprx");
		printf("ok!\n");
	} else {
		printf("copying the YAMT driver... ");
		fcp((data.version < 0x3630000) ? "app0:yamt_360.skprx" : "app0:yamt_365.skprx", "ur0:tai/yamt.skprx");
		fcp("app0:yamt.suprx", "ur0:tai/yamt.suprx");
		printf("ok!\n");
		psvDebugScreenSetFgColor(COLOR_YELLOW);
		printf("\npress SQUARE to install the USB/PSVSD driver\nor press any other key to continue\n");
		int addUSB = (get_key() == SCE_CTRL_SQUARE) ? 1 : 0;
		psvDebugScreenSetFgColor(COLOR_WHITE);
		if (addUSB) {
			printf("copying the USB compat driver... ");
			fcp("app0:usbuo.skprx", "ur0:tai/yamt_usb.skprx");
		}
		printf("ok!\nadding psp2 config lines... ");
		if (installYamtB() < 0) {
			psvDebugScreenSetFgColor(COLOR_RED);
			printf("failed...\n ");
			sceKernelDelayThread(4 * 1000 * 1000);
			sceKernelExitProcess(0);
			return -1;
		}
		printf("ok!\nadding tai config lines... ");
		installYamtC();
		sceIoRemove("ux0:tai/config.txt");
		printf("ok!\n");
	}
	psvDebugScreenSetFgColor(COLOR_GREEN);
	printf("\ndone, rebooting!\n");
	sceKernelDelayThread(4 * 1000 * 1000);
	vshPowerRequestColdReset();
	sceKernelExitProcess(0);
}
