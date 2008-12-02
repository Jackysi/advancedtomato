#include <linux/types.h>

enum lasat_mtdparts {
	LASAT_MTD_BOOTLOADER,
	LASAT_MTD_SERVICE,
	LASAT_MTD_NORMAL,
	LASAT_MTD_FS,
	LASAT_MTD_CONFIG,
	LASAT_MTD_LAST
}; 

#define BOOTLOADER_SIZE 0x40000

extern unsigned long lasat_flash_partition_start(int partno);
extern unsigned long lasat_flash_partition_size(int partno);
