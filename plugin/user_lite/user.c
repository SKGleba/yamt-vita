#include <psp2/kernel/clib.h>
#include <psp2/kernel/modulemgr.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/registrymgr.h>
#include <psp2/system_param.h>
#include <psp2/power.h>
#include <taihen.h>
#include <string.h>
#include "../kernel_lite/defs.h"
#include "../user/FatFormatProxy.h"
#include "language.h"

#define CONFIG_PATH "ur0:tai/yamt.cfg"

extern unsigned char _binary_peripherals_settings_xml_start;
extern unsigned char _binary_peripherals_settings_xml_size;

static lite_cfg_struct cfg;

static SceUID g_hooks[7];

int module_get_offset(SceUID modid, int segidx, uint32_t offset, void *stub_out){

	int res = 0;
	SceKernelModuleInfo info;

	if(segidx > 3){
		return -1;
	}

	if(stub_out == NULL){
		return -2;
	}

	res = sceKernelGetModuleInfo(modid, &info);
	if(res < 0){
		return res;
	}

	if(offset > info.segments[segidx].memsz){
		return -3;
	}

	*(uint32_t *)stub_out = (uint32_t)(info.segments[segidx].vaddr + offset);

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
	if (cfg.ver == 0x34 && rd == sizeof(cfg))
		return 0;
  }
  // default config
  sceClibMemset(&cfg, 0, sizeof(cfg));
  cfg.ver = 0x34;
  cfg.ion = 0x30;
  cfg.uxm = 0x30;
  cfg.umm = 0x30;
  save_config_user();
  return 0;
}

static void callCOp(int opid) {
	switch (opid) {
		case 1:
			sceIoRemove(CONFIG_PATH);
			load_config_user();
			break;
		case 2: // Format the SD card to exFAT
			formatBlkDev_settings("sdstor0:ext-lp-ign-entire", F_TYPE_EXFAT, 1);
			break;
		case 3:
			break;
		default:
			break;
	}
}

static tai_hook_ref_t g_sceRegMgrGetKeyInt_SceSystemSettingsCore_hook;
static int sceRegMgrGetKeyInt_SceSystemSettingsCore_patched(const char *category, const char *name, int *value) {
  if (sceClibStrncmp(category, "/CONFIG/YAMT", 12) == 0) {
    if (value) {
      load_config_user();
      if (sceClibStrncmp(name, "enable_driver", 13) == 0) {
        *value = cfg.ion - 0x30;
      } else if (sceClibStrncmp(name, "uxm", 3) == 0) {
		*value = cfg.uxm - 0x30;
	  } else if (sceClibStrncmp(name, "umm", 3) == 0) {
		*value = cfg.umm - 0x30;
	  } else if (sceClibStrncmp(name, "cop", 3) == 0) {
	    *value = 0;
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
      cfg.ion = value + 0x30;
    } else if (sceClibStrncmp(name, "uxm", 3) == 0) {
      cfg.uxm = value + 0x30;
    } else if (sceClibStrncmp(name, "umm", 3) == 0) {
      cfg.umm = value + 0x30;
	} else if (sceClibStrncmp(name, "cop", 3) == 0) {
	  callCOp(value);
	}
    save_config_user();
    return 0;
  }
  return TAI_CONTINUE(int, g_sceRegMgrSetKeyInt_SceSystemSettingsCore_hook, category, name, value);
}

static tai_hook_ref_t g_scePafToplevelGetText_SceSystemSettingsCore_hook;
static wchar_t *scePafToplevelGetText_SceSystemSettingsCore_patched(void *arg, char **msg) {
  language_container_t *language_container;
  int language = -1;
  sceRegMgrGetKeyInt("/CONFIG/SYSTEM", "language", &language);
  switch (language) {
    case SCE_SYSTEM_PARAM_LANG_JAPANESE:      language_container = &language_japanese;      break;
    case SCE_SYSTEM_PARAM_LANG_ENGLISH_US:    language_container = &language_english_us;    break;
    case SCE_SYSTEM_PARAM_LANG_FRENCH:        language_container = &language_french;        break;
    case SCE_SYSTEM_PARAM_LANG_SPANISH:       language_container = &language_spanish;       break;
    case SCE_SYSTEM_PARAM_LANG_GERMAN:        language_container = &language_german;        break;
    case SCE_SYSTEM_PARAM_LANG_ITALIAN:       language_container = &language_italian;       break;
    case SCE_SYSTEM_PARAM_LANG_DUTCH:         language_container = &language_dutch;         break;
    case SCE_SYSTEM_PARAM_LANG_PORTUGUESE_PT: language_container = &language_portuguese_pt; break;
    case SCE_SYSTEM_PARAM_LANG_RUSSIAN:       language_container = &language_russian;       break;
    case SCE_SYSTEM_PARAM_LANG_KOREAN:        language_container = &language_korean;        break;
    case SCE_SYSTEM_PARAM_LANG_CHINESE_T:     language_container = &language_chinese_t;     break;
    case SCE_SYSTEM_PARAM_LANG_CHINESE_S:     language_container = &language_chinese_s;     break;
    case SCE_SYSTEM_PARAM_LANG_FINNISH:       language_container = &language_finnish;       break;
    case SCE_SYSTEM_PARAM_LANG_SWEDISH:       language_container = &language_swedish;       break;
    case SCE_SYSTEM_PARAM_LANG_DANISH:        language_container = &language_danish;        break;
    case SCE_SYSTEM_PARAM_LANG_NORWEGIAN:     language_container = &language_norwegian;     break;
    case SCE_SYSTEM_PARAM_LANG_POLISH:        language_container = &language_polish;        break;
    case SCE_SYSTEM_PARAM_LANG_PORTUGUESE_BR: language_container = &language_portuguese_br; break;
    case SCE_SYSTEM_PARAM_LANG_ENGLISH_GB:    language_container = &language_english_gb;    break;
    case SCE_SYSTEM_PARAM_LANG_TURKISH:       language_container = &language_turkish;       break;
    default:                                  language_container = &language_english_us;    break;
  }
  if (msg && sceClibStrncmp(*msg, "msg_", 4) == 0) {
    #define LANGUAGE_ENTRY(name) \
      else if (sceClibStrncmp(*msg, #name, sizeof(#name)) == 0) { \
        return language_container->name; \
      }
    if (0) {}
    LANGUAGE_ENTRY(msg_storage_devices)
    LANGUAGE_ENTRY(msg_use_yamt)
    LANGUAGE_ENTRY(msg_enable_yamt)
    LANGUAGE_ENTRY(msg_default)
    LANGUAGE_ENTRY(msg_sd2vita)
    LANGUAGE_ENTRY(msg_memory_card)
    LANGUAGE_ENTRY(msg_internal_storage)
    LANGUAGE_ENTRY(msg_developer_options)
    LANGUAGE_ENTRY(msg_idle)
	LANGUAGE_ENTRY(msg_reset_yamt)
	LANGUAGE_ENTRY(msg_format_gcsd)
	LANGUAGE_ENTRY(msg_rw_mount_sa0_pd0)
    #undef LANGUAGE_ENTRY
  }
  return TAI_CONTINUE(wchar_t *, g_scePafToplevelGetText_SceSystemSettingsCore_hook, arg, msg);
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
	g_hooks[6] = taiHookFunctionImport(&g_scePafToplevelGetText_SceSystemSettingsCore_hook, 
                                        "SceSystemSettingsCore", 
                                        0x4D9A9DD0, // ScePafToplevel
                                        0x19CEFDA7, 
                                        scePafToplevelGetText_SceSystemSettingsCore_patched);
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
    if (g_hooks[6] >= 0) taiHookRelease(g_hooks[6], g_scePafToplevelGetText_SceSystemSettingsCore_hook);
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
