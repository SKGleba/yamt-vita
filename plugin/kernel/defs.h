typedef struct cfg_entry {
  size_t t_off;
  uint8_t mid; // mountpoint id / 0x100
  uint8_t reqs[2]; // req, t/f
  uint8_t devn[4];
} __attribute__((packed)) cfg_entry;

typedef struct cfg_struct {
  uint8_t ver;
  uint8_t vec;
  uint8_t ion;
  uint16_t mrq;
  cfg_entry entry[16];
} __attribute__((packed)) cfg_struct;

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
