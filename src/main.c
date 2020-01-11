#include <psp2/kernel/processmgr.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/devctl.h>
#include <psp2/sysmodule.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/io/stat.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <psp2/kernel/modulemgr.h> 

#include "debug_screen.h"

#define printf psvDebugScreenPrintf

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

int verifyamted(char *txtloc) {
	char temp[7];
	SceUID fd = sceIoOpen(txtloc, SCE_O_RDONLY, 6);
	if (fd < 0)
		return 0;
	sceIoRead(fd, &temp, 7);
	sceIoClose(fd);
	if (sceClibStrncmp((void *)temp, "# YAMT\n", 7) == 0)
		return 1;
	return 0;
}

int main(int argc, char *argv[]) {
	(void)argc;
	(void)argv;

	psvDebugScreenInit();
	
	static char version[16];
	
	SceKernelFwInfo data;
	data.size = sizeof(SceKernelFwInfo);
	_vshSblGetSystemSwVersion(&data);
	
	if (ex("ur0:tai/boot_config.txt") == 0) {
		printf("Could not find boot_config.txt in ur0:tai/ !\n");
		sceKernelDelayThread(3 * 1000 * 1000);
		sceKernelExitProcess(0);
		sceKernelDelayThread(3 * 1000 * 1000);
	}
	
	int isbcfg = verifyamted("ur0:tai/boot_config.txt");
	int istcfg = verifyamted("ur0:tai/config.txt");

	if (ex("ur0:tai/yamt.skprx") == 0) {
		printf("Could not find yamt.skprx in ur0:tai/ !\n");
		printf("copying... ");
		fcp((data.version < 0x3630000) ? "app0:yamt_360.skprx" : "app0:yamt_365.skprx", "ur0:tai/yamt.skprx");
		printf("ok!\n");
		if (isbcfg == 0) {
			printf("adding entry to ur0:tai/boot_config.txt... ");
			SceUID fd = sceIoOpen("ur0:tai/boot_config_temp.txt", SCE_O_WRONLY | SCE_O_TRUNC | SCE_O_CREAT, 6);
			sceIoWrite(fd, (void *)"# YAMT\n- load\tur0:tai/yamt.skprx\n\n", strlen("# YAMT\n- load\tur0:tai/yamt.skprx\n\n"));
			sceIoClose(fd);
			fap("ur0:tai/boot_config.txt", "ur0:tai/boot_config_temp.txt");
			if (ex("ur0:tai/yamt_old_cfg.txt") == 1) 
				sceIoRemove("ur0:tai/yamt_old_cfg.txt");
			sceIoRename("ur0:tai/boot_config.txt", "ur0:tai/yamt_old_cfg.txt");
			sceIoRename("ur0:tai/boot_config_temp.txt", "ur0:tai/boot_config.txt");
			printf("ok!\n");
		}
		if (ex("ur0:tai/yamt.suprx") == 0) {
			printf("Could not find yamt.suprx in ur0:tai/ !\n");
			printf("copying... ");
			fcp("app0:yamt.suprx", "ur0:tai/yamt.suprx");
			printf("ok!\n");
			if (istcfg == 0) {
				printf("adding entry to ur0:tai/config.txt... ");
				SceUID fd = sceIoOpen("ur0:tai/config_temp.txt", SCE_O_WRONLY | SCE_O_TRUNC | SCE_O_CREAT, 6);
				sceIoWrite(fd, (void *)"# YAMT\n*NPXS10015\nur0:tai/yamt.suprx\n\n", strlen("# YAMT\n*NPXS10015\nur0:tai/yamt.suprx\n\n"));
				sceIoClose(fd);
				fap("ur0:tai/config.txt", "ur0:tai/config_temp.txt");
				if (ex("ur0:tai/config_preyamt.txt") == 1) 
					sceIoRemove("ur0:tai/config_preyamt.txt");
				sceIoRename("ur0:tai/config.txt", "ur0:tai/config_preyamt.txt");
				sceIoRename("ur0:tai/config_temp.txt", "ur0:tai/config.txt");
				printf("ok!\n");
			}
		}
	} else {
		printf("Found yamt.skprx in ur0:tai/ !\n");
		printf("removing... ");
		sceIoRemove("ur0:tai/yamt.skprx");
		printf("ok!\n");
		if (ex("ur0:tai/yamt.suprx") == 1) {
			printf("Found yamt.suprx in ur0:tai/ !\n");
			printf("removing... ");
			sceIoRemove("ur0:tai/yamt.suprx");
			printf("ok!\n");
		}
	}
	
	printf("done!\n");
	sceKernelDelayThread(4 * 1000 * 1000);
	sceKernelExitProcess(0);
}