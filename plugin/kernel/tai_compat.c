#include "tai_compat.h"

static int sce_to_tai_module_info(SceUID pid, void *sceinfo, tai_module_info_t *taiinfo) {
  char *info;
  if (taiinfo->size < sizeof(tai_module_info_t)) {
    return -1;
  }
  info = (char *)sceinfo;
    taiinfo->modid = *(SceUID *)(info + 0xC);
    snprintf(taiinfo->name, 27, "%s", *(const char **)(info + 0x1C));
    taiinfo->name[26] = '\0';
    taiinfo->module_nid = *(uint32_t *)(info + 0x30);
    taiinfo->exports_start = *(uintptr_t *)(info + 0x20);
    taiinfo->exports_end = *(uintptr_t *)(info + 0x24);
    taiinfo->imports_start = *(uintptr_t *)(info + 0x28);
    taiinfo->imports_end = *(uintptr_t *)(info + 0x2C);
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
  ret = ksceKernelGetModuleInfo(pid, modid, &sceinfo);
  if (ret < 0) {
    return ret;
  }
  if (offset > sceinfo.segments[segidx].memsz) {
    return -1;
  }
  *addr = (uintptr_t)sceinfo.segments[segidx].vaddr + offset;
  return 1;
}

int module_get_by_name_nid(SceUID pid, const char *name, tai_module_info_t *info) {
  SceUID modlist[MOD_LIST_SIZE];
  void *sceinfo;
  size_t count;
  int ret;
  int get_cur;
  uint32_t nid = TAI_IGNORE_MODULE_NID;
  get_cur = (name == NULL && nid == TAI_IGNORE_MODULE_NID);
  count = MOD_LIST_SIZE;
  ret = ksceKernelGetModuleList(pid, 0xff, 1, modlist, &count);
  if (ret < 0) {
    return ret;
  } else if (count == MOD_LIST_SIZE) {
    return -1;
  }
  for (int i = (count - 1); i >= 0; i--) {
    ret = ksceKernelGetModuleInternal(modlist[i], &sceinfo);
    if (ret < 0) {
      return ret;
    }
    if ((ret = sce_to_tai_module_info(pid, sceinfo, info)) < 0) {
      return ret;
    }
    if (name != NULL && strncmp(name, info->name, 27) == 0) {
      if (nid == TAI_IGNORE_MODULE_NID || info->module_nid == nid) {
        return 1;
      }
    }
  }
  return -2;
}
