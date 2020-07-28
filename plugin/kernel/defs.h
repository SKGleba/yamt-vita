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
  uint8_t drv[4];
  cfg_entry entry[16];
} __attribute__((packed)) cfg_struct;

typedef struct {
	const char *dev;
	const char *dev2;
	const char *blkdev;
	const char *blkdev2;
	int id;
} SceIoDevice;

typedef struct usercmd {
  uint32_t magic;
  uint32_t arg0;
  uint32_t arg1;
  uint32_t arg2;
  char data[0x200];
} __attribute__((packed)) usercmd;

typedef struct {
	int id;
	const char *dev_unix;
	int unk;
	int dev_major;
	int dev_minor;
	const char *dev_filesystem;
	int unk2;
	SceIoDevice *dev;
	int unk3;
	SceIoDevice *dev2;
	int unk4;
	int unk5;
	int unk6;
	int unk7;
} SceIoMountPoint;

static char *st[] = {
	"int",
	"ext",
	"gcd",
	"mcd",
	"xmc",
	"uma"
};
static char *rd[] = {
	"ina",
	"act",
	"ign"
};
static char *th[] = {
	"a",
	"unused",
	"idstor",
	"sloader",
	"os",
	"vsh",
	"vshdata",
	"vtrm",
	"user",
	"userext",
	"gamero",
	"gamerw",
	"updater",
	"sysdata",
	"mediaid",
	"pidata",
	"entire"
};
