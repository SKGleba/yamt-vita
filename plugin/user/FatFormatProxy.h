// Supported FAT types
#define F_TYPE_FAT32 0x5 // or 0x8 or 0xB
#define F_TYPE_FAT16 0x6
#define F_TYPE_EXFAT 0x7

/* 
 * patch FatFormat::devFdWrite args created by FatFormatTranslateArgs
 * 	this patch makes devFdWrite write the partition to device offset 0x0
 * 	instead of writing to blockAllign * 0x200
 * 
 * TODO: proper args RE
*/
static tai_hook_ref_t f_create_args_ref;
int f_create_args_patched(void *old_ff_args, void *ff_args, int mode) {
  int ret = TAI_CONTINUE(int, f_create_args_ref, old_ff_args, ff_args, mode);
  *(uint32_t *)(ff_args + (7 * 4)) = 0; // newFatFormatArgs->safeStartBlockAlligned
  return ret;
}

/*
 * formats [dev] to [fst]
 * set [external] to 1 if the partition should be written to master device offset 0x0
*/
static int formatBlkDev_settings(char *dev, int fst, int external) {
	int (* FatFormatProxy)(char *devblk, int ptype, uint32_t unk2, uint32_t unk3, char *unk4, char *unk5) = NULL;
	
	// settings have FatFormat & proxy embedded
	tai_module_info_t info;
	info.size = sizeof(info);
	if(taiGetModuleInfo("SceSettings", &info) < 0)
		return -1;
	
	if (module_get_offset(info.modid, 0, 0x15b20d, &FatFormatProxy) < 0)
		return -1;
		
	int goodfw = (*(uint16_t *)FatFormatProxy == 0xf0e9) ? 1 : 0; // easy way to check fw ver
	
	if (!goodfw)
		module_get_offset(info.modid, 0, 0x15b209, &FatFormatProxy);
	
	// external device patch
	int create_args_uid = 0;
	if (external)
		create_args_uid = taiHookFunctionOffset(&f_create_args_ref, info.modid, 0, (goodfw) ? 0x15e752 : 0x15e74e, 1, f_create_args_patched);
		
	// format the device
	int ret = FatFormatProxy(dev, fst, 0x8000, 0x8000, NULL, NULL);
	
	if (external)
		taiHookRelease(create_args_uid, f_create_args_ref);
	
	return ret;
}
