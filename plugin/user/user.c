#include <psp2/kernel/clib.h>
#include <psp2/kernel/modulemgr.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/registrymgr.h>
#include <psp2/system_param.h>
#include <psp2/power.h>
#include <taihen.h>
#include <string.h>

#define CONFIG_PATH "ur0:tai/yamt.cfg"

typedef struct cfg_entry {
  size_t t_off;
  uint8_t mid; // mountpoint id / 0x100
  uint8_t mode;
  uint8_t reqs[2]; // req, t/f
  uint8_t devn[4];
} __attribute__((packed)) cfg_entry;

typedef struct cfg_struct {
  uint8_t ver;
  uint8_t vec;
  uint8_t ion;
  uint8_t mrq;
  cfg_entry entry[16];
} __attribute__((packed)) cfg_struct;

extern unsigned char _binary_peripherals_settings_xml_start;
extern unsigned char _binary_peripherals_settings_xml_size;

static cfg_struct cfg;

static SceUID g_hooks[6];

static void set_slots(void) {
	cfg.entry[0].t_off = 0x808; // XMCUX
	cfg.entry[0].mid = 0x8;
	cfg.entry[1].t_off = 0x43C; // UMA
	cfg.entry[1].mid = 0xF;
	cfg.entry[2].t_off = 0x164; // SD0
	cfg.entry[2].mid = 0x1;
	cfg.entry[3].t_off = 0x72C; // OS0
	cfg.entry[3].mid = 0x2;
	cfg.entry[4].t_off = 0x60C; // VS0
	cfg.entry[4].mid = 0x3;
	cfg.entry[5].t_off = 0x1B4; // VD0
	cfg.entry[5].mid = 0x4;
	cfg.entry[6].t_off = 0x60; // TM0
	cfg.entry[6].mid = 0x5;
	cfg.entry[7].t_off = 0xEC; // UR0
	cfg.entry[7].mid = 0x6;
	cfg.entry[8].t_off = 0x28C; // UD0
	cfg.entry[8].mid = 0x7;
	cfg.entry[9].t_off = 0x18; // GRO0
	cfg.entry[9].mid = 0x9;
	cfg.entry[10].t_off = 0x40C; // GRW0
	cfg.entry[10].mid = 0xA;
	cfg.entry[11].t_off = 0x370; // SA0
	cfg.entry[11].mid = 0xB;
	cfg.entry[12].t_off = 0x4DC; // PD0
	cfg.entry[12].mid = 0xC;
	cfg.entry[13].t_off = 0xDEADBEEF; // IMC0
	cfg.entry[13].mid = 0xD;
	cfg.entry[14].t_off = 0xDEADBEEF; // XMC0
	cfg.entry[14].mid = 0xE;
	int lpos = 0;
	while (lpos < 16) {
		sceClibMemset(&cfg.entry[lpos].devn, 0xFF, sizeof(cfg.entry[lpos].devn));
		cfg.entry[lpos].mode = 1;
		lpos = lpos + 1;
	}
}

static void set_cfg_etr_ez(int slot, int dno) {
	if (dno == 0) { // DEFAULT
		sceClibMemset(&cfg.entry[slot].devn, 0xFF, sizeof(cfg.entry[slot].devn));
	} else if (dno == 1) { // sd2vita
		cfg.entry[slot].devn[0] = 2;
		cfg.entry[slot].devn[1] = 0;
		cfg.entry[slot].devn[2] = 1;
		cfg.entry[slot].devn[3] = 16;
		cfg.entry[slot].reqs[0] = 1;
		cfg.entry[slot].reqs[1] = 1;
	} else if (dno == 2) {
		cfg.entry[slot].devn[0] = 4;
		cfg.entry[slot].devn[1] = 0;
		cfg.entry[slot].devn[2] = 2;
		cfg.entry[slot].devn[3] = 9;
		cfg.entry[slot].reqs[0] = 2;
		cfg.entry[slot].reqs[1] = 1;
	} else if (dno == 3) {
		cfg.entry[slot].devn[0] = 0;
		cfg.entry[slot].devn[1] = 0;
		cfg.entry[slot].devn[2] = 2;
		cfg.entry[slot].devn[3] = 9;
		cfg.entry[slot].reqs[0] = 0;
		cfg.entry[slot].reqs[1] = 0;
	}
}

static int get_cfg_etr_ez(int slot) {
	if (cfg.entry[slot].devn[0] == 2) {
		return 1;
	} else if (cfg.entry[slot].devn[0] == 4) {
		return 2;
	} else if (cfg.entry[slot].devn[0] == 0) {
		return 3;
	} else if (cfg.entry[slot].devn[0] == 0xFF) {
		return 0;
	}
	return 0;
}

static void save_config_user(void) {
  SceUID fd;
  fd = sceIoOpen(CONFIG_PATH, SCE_O_TRUNC | SCE_O_CREAT | SCE_O_WRONLY, 6);
  if (fd >= 0) {
    sceIoWrite(fd, &cfg, sizeof(cfg));
    sceIoClose(fd);
  }
}

static int load_config_user(void) {
  SceUID fd;
  int rd;
  fd = sceIoOpen(CONFIG_PATH, SCE_O_RDONLY, 0);
  if (fd >= 0) {
    rd = sceIoRead(fd, &cfg, sizeof(cfg));
    sceIoClose(fd);
	if (cfg.ver == 3 && rd == sizeof(cfg))
		return 0;
  }
  // default config
  sceClibMemset(&cfg, 0, sizeof(cfg));
  cfg.ver = 3;
  cfg.vec = 16;
  cfg.ion = 0;
  cfg.mrq = 1;
  set_slots();
  save_config_user();
  return 0;
}

static int get_cfg_ux_off(void) {
	if (cfg.entry[0].t_off == 0x808)
		return 1;
	return 0;
}

static void set_cfg_ux_off(int offc) {
	if (offc == 1) {
		cfg.entry[0].t_off = 0x808; // XMC
	} else {
		cfg.entry[0].t_off = 0x340; // IMC
	}
}

static tai_hook_ref_t g_sceRegMgrGetKeyInt_SceSystemSettingsCore_hook;
static int sceRegMgrGetKeyInt_SceSystemSettingsCore_patched(const char *category, const char *name, int *value) {
  if (sceClibStrncmp(category, "/CONFIG/YAMT", 12) == 0) {
    if (value) {
      load_config_user();
      if (sceClibStrncmp(name, "enable_driver", 13) == 0) {
        *value = cfg.ion;
	  } else if (sceClibStrncmp(name, "enable_gcsd", 11) == 0) {
        *value = cfg.mrq;
      } else if (sceClibStrncmp(name, "uxm", 3) == 0) {
		*value = get_cfg_etr_ez(0);
	  } else if (sceClibStrncmp(name, "umm", 3) == 0) {
		*value = get_cfg_etr_ez(1);
	  } else if (sceClibStrncmp(name, "uxpmode", 7) == 0) {
		*value = get_cfg_ux_off();
      } else if (sceClibStrncmp(name, "adv_dev_", 8) == 0) {
		*value = cfg.entry[((name[8] - 0x30) * 10) + (name[9] - 0x30)].devn[name[10] - 0x30];
      } else if (sceClibStrncmp(name, "adv_rchk_", 9) == 0) {
		*value = cfg.entry[((name[9] - 0x30) * 10) + (name[10] - 0x30)].reqs[0];
      } else if (sceClibStrncmp(name, "adv_rcret_", 10) == 0) {
		*value = cfg.entry[((name[10] - 0x30) * 10) + (name[11] - 0x30)].reqs[1];
	  } else if (sceClibStrncmp(name, "adv_mode_", 9) == 0) {
		*value = cfg.entry[((name[9] - 0x30) * 10) + (name[10] - 0x30)].mode;
      }
    }
    return 0;
  }
  return TAI_CONTINUE(int, g_sceRegMgrGetKeyInt_SceSystemSettingsCore_hook, category, name, value);
}

static tai_hook_ref_t g_sceRegMgrSetKeyInt_SceSystemSettingsCore_hook;
static int sceRegMgrSetKeyInt_SceSystemSettingsCore_patched(const char *category, const char *name, int value) {
  if (sceClibStrncmp(category, "/CONFIG/YAMT", 12) == 0) {
    if (sceClibStrncmp(name, "enable_driver", 13) == 0) {
      cfg.ion = value;
    } else if (sceClibStrncmp(name, "enable_gcsd", 11) == 0) {
      cfg.mrq = value;
    } else if (sceClibStrncmp(name, "uxm", 3) == 0) {
      set_cfg_etr_ez(0, value);
    } else if (sceClibStrncmp(name, "umm", 3) == 0) {
      set_cfg_etr_ez(1, value);
	} else if (sceClibStrncmp(name, "uxpmode", 7) == 0) {
      set_cfg_ux_off(value);
    } else if (sceClibStrncmp(name, "adv_dev_", 8) == 0) {
	  cfg.entry[((name[8] - 0x30) * 10) + (name[9] - 0x30)].devn[name[10] - 0x30] = value;
    } else if (sceClibStrncmp(name, "adv_rchk_", 9) == 0) {
	  cfg.entry[((name[9] - 0x30) * 10) + (name[10] - 0x30)].reqs[0] = value;
    } else if (sceClibStrncmp(name, "adv_rcret_", 10) == 0) {
	  cfg.entry[((name[10] - 0x30) * 10) + (name[11] - 0x30)].reqs[1] = value;
	} else if (sceClibStrncmp(name, "adv_mode_", 9) == 0) {
	  cfg.entry[((name[9] - 0x30) * 10) + (name[10] - 0x30)].mode = value;
    }
    save_config_user();
    return 0;
  }
  return TAI_CONTINUE(int, g_sceRegMgrSetKeyInt_SceSystemSettingsCore_hook, category, name, value);
}

typedef struct {
  int size;
  const char *name;
  int type;
  int unk;
} SceRegMgrKeysInfo;

static tai_hook_ref_t g_sceRegMgrGetKeysInfo_SceSystemSettingsCore_hook;
static int sceRegMgrGetKeysInfo_SceSystemSettingsCore_patched(const char *category, SceRegMgrKeysInfo *info, int unk) {
  if (sceClibStrncmp(category, "/CONFIG/YAMT", 12) == 0) {
    if (info) {
        info->type = 0x00040000; // type integer
    }
    return 0;
  }
  return TAI_CONTINUE(int, g_sceRegMgrGetKeysInfo_SceSystemSettingsCore_hook, category, info, unk);
}

static tai_hook_ref_t g_scePafMiscLoadXmlLayout_SceSettings_hook;
static int scePafMiscLoadXmlLayout_SceSettings_patched(int a1, void *xml_buf, int xml_size, int a4) {
  if (sceClibStrncmp(xml_buf+85, "peripherals_settings_plugin", 27) == 0) {
    xml_buf = (void *)&_binary_peripherals_settings_xml_start;
    xml_size = (int)&_binary_peripherals_settings_xml_size;
  }
  return TAI_CONTINUE(int, g_scePafMiscLoadXmlLayout_SceSettings_hook, a1, xml_buf, xml_size, a4);
}

static SceUID g_system_settings_core_modid = -1;
static tai_hook_ref_t g_sceKernelLoadStartModule_SceSettings_hook;
static SceUID sceKernelLoadStartModule_SceSettings_patched(char *path, SceSize args, void *argp, int flags, SceKernelLMOption *option, int *status) {
  SceUID ret = TAI_CONTINUE(SceUID, g_sceKernelLoadStartModule_SceSettings_hook, path, args, argp, flags, option, status);
  if (ret >= 0 && sceClibStrncmp(path, "vs0:app/NPXS10015/system_settings_core.suprx", 44) == 0) {
    g_system_settings_core_modid = ret;
    g_hooks[2] = taiHookFunctionImport(&g_scePafMiscLoadXmlLayout_SceSettings_hook, 
                                        "SceSettings", 
                                        0x3D643CE8, // ScePafMisc
                                        0x19FE55A8, 
                                        scePafMiscLoadXmlLayout_SceSettings_patched);
    g_hooks[3] = taiHookFunctionImport(&g_sceRegMgrGetKeyInt_SceSystemSettingsCore_hook, 
                                        "SceSystemSettingsCore", 
                                        0xC436F916, // SceRegMgr
                                        0x16DDF3DC, 
                                        sceRegMgrGetKeyInt_SceSystemSettingsCore_patched);
    g_hooks[4] = taiHookFunctionImport(&g_sceRegMgrSetKeyInt_SceSystemSettingsCore_hook, 
                                        "SceSystemSettingsCore", 
                                        0xC436F916, // SceRegMgr
                                        0xD72EA399, 
                                        sceRegMgrSetKeyInt_SceSystemSettingsCore_patched);
    g_hooks[5] = taiHookFunctionImport(&g_sceRegMgrGetKeysInfo_SceSystemSettingsCore_hook, 
                                        "SceSystemSettingsCore", 
                                        0xC436F916, // SceRegMgr
                                        0x58421DD1, 
                                        sceRegMgrGetKeysInfo_SceSystemSettingsCore_patched);
  }
  return ret;
}

static tai_hook_ref_t g_sceKernelStopUnloadModule_SceSettings_hook;
static int sceKernelStopUnloadModule_SceSettings_patched(SceUID modid, SceSize args, void *argp, int flags, SceKernelULMOption *option, int *status) {
  if (modid == g_system_settings_core_modid) {
    g_system_settings_core_modid = -1;
    if (g_hooks[2] >= 0) taiHookRelease(g_hooks[2], g_scePafMiscLoadXmlLayout_SceSettings_hook);
    if (g_hooks[3] >= 0) taiHookRelease(g_hooks[3], g_sceRegMgrGetKeyInt_SceSystemSettingsCore_hook);
    if (g_hooks[4] >= 0) taiHookRelease(g_hooks[4], g_sceRegMgrSetKeyInt_SceSystemSettingsCore_hook);
    if (g_hooks[5] >= 0) taiHookRelease(g_hooks[5], g_sceRegMgrGetKeysInfo_SceSystemSettingsCore_hook);
  }
  return TAI_CONTINUE(int, g_sceKernelStopUnloadModule_SceSettings_hook, modid, args, argp, flags, option, status);
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args) {
  sceClibMemset(&cfg, 0, sizeof(cfg));
  load_config_user();
  g_hooks[0] = taiHookFunctionImport(&g_sceKernelLoadStartModule_SceSettings_hook, 
                                      "SceSettings", 
                                      0xCAE9ACE6, // SceLibKernel
                                      0x2DCC4AFA, 
                                      sceKernelLoadStartModule_SceSettings_patched);
  g_hooks[1] = taiHookFunctionImport(&g_sceKernelStopUnloadModule_SceSettings_hook, 
                                      "SceSettings", 
                                      0xCAE9ACE6, // SceLibKernel
                                      0x2415F8A4, 
                                      sceKernelStopUnloadModule_SceSettings_patched);
  return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {
  // free hooks that didn't fail
  if (g_hooks[0] >= 0) taiHookRelease(g_hooks[0], g_sceKernelLoadStartModule_SceSettings_hook);
  if (g_hooks[1] >= 0) taiHookRelease(g_hooks[1], g_sceKernelStopUnloadModule_SceSettings_hook);
  if (g_hooks[2] >= 0) taiHookRelease(g_hooks[2], g_scePafMiscLoadXmlLayout_SceSettings_hook);
  if (g_hooks[3] >= 0) taiHookRelease(g_hooks[3], g_sceRegMgrGetKeyInt_SceSystemSettingsCore_hook);
  if (g_hooks[4] >= 0) taiHookRelease(g_hooks[4], g_sceRegMgrSetKeyInt_SceSystemSettingsCore_hook);
  if (g_hooks[5] >= 0) taiHookRelease(g_hooks[5], g_sceRegMgrGetKeysInfo_SceSystemSettingsCore_hook);
  return SCE_KERNEL_STOP_SUCCESS;
}
