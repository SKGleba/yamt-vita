#include "tai_compat.h"

static int (* get_mod_info)(SceUID pid, SceUID modid, SceKernelModuleInfo *sceinfo_op) = NULL;

int tai_init(void) {
	char stuz[4];
	stuz[0] = *(uint8_t *)ksceKernelSearchModuleByName;
	stuz[1] = (*(uint8_t *)(ksceKernelSearchModuleByName + 1) - 0xC0) + (*(uint8_t *)(ksceKernelSearchModuleByName + 2) * 0x10);
	stuz[2] = *(uint8_t *)(ksceKernelSearchModuleByName + 4);
	stuz[3] = (*(uint8_t *)(ksceKernelSearchModuleByName + 5) - 0xC0) + (*(uint8_t *)(ksceKernelSearchModuleByName + 6) - 0x40);
	get_mod_info = (void *)(*(uint32_t *)stuz) + 0x30b4; // ksceKernelSearchModuleByName is @ 0x3d00
	if (*(uint16_t *)get_mod_info != 0x83b5) {
		get_mod_info = (void *)(*(uint32_t *)stuz) + 0x476c;
		if (*(uint16_t *)get_mod_info != 0x83b5)
			return -1;
		return 0;
	}
	return 1;
}
int module_get_offset(SceUID pid, SceUID modid, int segidx, size_t offset, uintptr_t *addr) {
  SceKernelModuleInfo sceinfo;
  size_t count;
  int ret;
  if (segidx > 3) {
    return -1;
  }
  sceinfo.size = sizeof(sceinfo);
  if (get_mod_info == NULL) {
	  if (tai_init() < 0)
		  return -1;
  }
  ret = get_mod_info(pid, modid, &sceinfo);
  if (ret < 0) {
    return ret;
  }
  if (offset > sceinfo.segments[segidx].memsz) {
    return -1;
  }
  *addr = (uintptr_t)sceinfo.segments[segidx].vaddr + offset;
  return 1;
}


