static int enable_logging = LOGGING_ENABLED;
static char buf_logging[256];

#define LOG(...) \
	do { \
		ksceDebugPrintf(__VA_ARGS__); \
		if (enable_logging == 1) { \
			memset(buf_logging, 0, sizeof(buf_logging)); \
			snprintf(buf_logging, sizeof(buf_logging), ##__VA_ARGS__); \
			logg(buf_logging, strlen(buf_logging), LOG_LOC, 2); \
		}; \
} while (0)
	
#define LOG_START(...) \
	do { \
		ksceDebugPrintf(__VA_ARGS__); \
		if (enable_logging == 1) { \
			memset(buf_logging, 0, sizeof(buf_logging)); \
			snprintf(buf_logging, sizeof(buf_logging), ##__VA_ARGS__); \
			logg(buf_logging, strlen(buf_logging), LOG_LOC, 1); \
		}; \
} while (0)

static int logg(void *buffer, int length, const char* logloc, int create)
{
	int fd;
	if (create == 0) {
		fd = ksceIoOpen(logloc, SCE_O_WRONLY | SCE_O_APPEND, 6);
	} else if (create == 1) {
		fd = ksceIoOpen(logloc, SCE_O_WRONLY | SCE_O_TRUNC | SCE_O_CREAT, 6);
	} else if (create == 2) {
		fd = ksceIoOpen(logloc, SCE_O_WRONLY | SCE_O_APPEND | SCE_O_CREAT, 6);
	}
	if (fd < 0)
		return 0;
	ksceIoWrite(fd, buffer, length);
	ksceIoClose(fd);
	return 1;
}
