/*

	Copyright 2003, CyberTAN  Inc.  All Rights Reserved

	This is UNPUBLISHED PROPRIETARY SOURCE CODE of CyberTAN Inc.
	the contents of this file may not be disclosed to third parties,
	copied or duplicated in any form without the prior written
	permission of CyberTAN Inc.

	This software should be used as a reference only, and it not
	intended for production use!

	THIS SOFTWARE IS OFFERED "AS IS", AND CYBERTAN GRANTS NO WARRANTIES OF ANY
	KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE.  CYBERTAN
	SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
	FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE

*/
/*

	Copyright 2005, Broadcom Corporation
	All Rights Reserved.

	THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
	KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
	SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
	FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.

*/
/*

	Modified for Tomato Firmware
	Portions, Copyright (C) 2006-2009 Jonathan Zarate

*/

#include "rc.h"

#include <limits.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <error.h>
#include <sys/ioctl.h>
#include <sys/sysinfo.h>
#ifdef LINUX26
#include <linux/compiler.h>
#include <mtd/mtd-user.h>
#else
#include <linux/mtd/mtd.h>
#endif
#include <stdint.h>

#include <trxhdr.h>
#include <bcmutils.h>


//	#define DEBUG_SIMULATE


struct code_header {
	char magic[4];
	char res1[4];
	char fwdate[3];
	char fwvern[3];
	char id[4];
	char hw_ver;
	char res2;
	unsigned short flags;
	unsigned char res3[10];
} ;

// -----------------------------------------------------------------------------

static uint32 *crc_table = NULL;

static void crc_done(void)
{
	free(crc_table);
	crc_table = NULL;
}

static int crc_init(void)
{
	uint32 c;
	int i, j;

	if (crc_table == NULL) {
		if ((crc_table = malloc(sizeof(uint32) * 256)) == NULL) return 0;
		for (i = 255; i >= 0; --i) {
			c = i;
			for (j = 8; j > 0; --j) {
				if (c & 1) c = (c >> 1) ^ 0xEDB88320L;
					else c >>= 1;
			}
			crc_table[i] = c;
		}
	}
	return 1;
}

static uint32 crc_calc(uint32 crc, char *buf, int len)
{
	while (len-- > 0) {
		crc = crc_table[(crc ^ *((char *)buf)) & 0xFF] ^ (crc >> 8);
		buf++;
	}
	return crc;
}

// -----------------------------------------------------------------------------

static int mtd_open(const char *mtdname, mtd_info_t *mi)
{
	char path[256];
	int part;
	int size;
	int f;

	if (mtd_getinfo(mtdname, &part, &size)) {
		sprintf(path, MTD_DEV(%d), part);
		if ((f = open(path, O_RDWR|O_SYNC)) >= 0) {
			if ((mi) && ioctl(f, MEMGETINFO, mi) != 0) {
				close(f);
				return -1;
			}
			return f;
		}
	}
	return -1;
}

static int _unlock_erase(const char *mtdname, int erase)
{
	int mf;
	mtd_info_t mi;
	erase_info_t ei;
	int r;

	if (!wait_action_idle(5)) return 0;
	set_action(ACT_ERASE_NVRAM);
	if (erase) led(LED_DIAG, 1);

	r = 0;
	if ((mf = mtd_open(mtdname, &mi)) >= 0) {
			r = 1;
#if 1
			ei.length = mi.erasesize;
			for (ei.start = 0; ei.start < mi.size; ei.start += mi.erasesize) {
				printf("%sing 0x%x - 0x%x\n", erase ? "Eras" : "Unlock", ei.start, (ei.start + ei.length) - 1);
				fflush(stdout);

				if (ioctl(mf, MEMUNLOCK, &ei) != 0) {
//					perror("MEMUNLOCK");
//					r = 0;
//					break;
				}
				if (erase) {
					if (ioctl(mf, MEMERASE, &ei) != 0) {
						perror("MEMERASE");
						r = 0;
						break;
					}
				}
			}
#else
			ei.start = 0;
			ei.length = mi.size;

			printf("%sing 0x%x - 0x%x\n", erase ? "Eras" : "Unlock", ei.start, ei.length - 1);
			fflush(stdout);

			if (ioctl(mf, MEMUNLOCK, &ei) != 0) {
				perror("MEMUNLOCK");
				r = 0;
			}
			else if (erase) {
				if (ioctl(mf, MEMERASE, &ei) != 0) {
					perror("MEMERASE");
					r = 0;
				}
			}
#endif

			// checkme:
			char buf[2];
			read(mf, &buf, sizeof(buf));
			close(mf);
	}

	if (erase) led(LED_DIAG, 0);
	set_action(ACT_IDLE);

	if (r) printf("\"%s\" successfully %s.\n", mtdname, erase ? "erased" : "unlocked");
        else printf("\nError %sing MTD\n", erase ? "eras" : "unlock");

	sleep(1);
	return r;
}

int mtd_unlock(const char *mtdname)
{
	return _unlock_erase(mtdname, 0);
}

int mtd_erase(const char *mtdname)
{
	return _unlock_erase(mtdname, 1);
}

int mtd_unlock_erase_main(int argc, char *argv[])
{
	char c;
	char *dev = NULL;

	while ((c = getopt(argc, argv, "d:")) != -1) {
		switch (c) {
		case 'd':
			dev = optarg;
			break;
		}
	}

	if (!dev) {
		usage_exit(argv[0], "-d part");
	}

	return _unlock_erase(dev, strstr(argv[0], "erase") ? 1 : 0);
}

int mtd_write_main(int argc, char *argv[])
{
	int mf = -1;
	mtd_info_t mi;
	erase_info_t ei;
	uint32 sig;
	struct trx_header trx;
	struct code_header cth;
	uint32 crc;
	FILE *f;
	char *buf = NULL;
	const char *error;
	uint32 total;
	uint32 n;
	struct sysinfo si;
	uint32 ofs;
	char c;
	int web = 0;
	char *iname = NULL;
	char *dev = NULL;
	int model;

	while ((c = getopt(argc, argv, "i:d:w")) != -1) {
		switch (c) {
		case 'i':
			iname = optarg;
			break;
		case 'd':
			dev = optarg;
			break;
		case 'w':
			web = 1;
			break;
		}
	}

	if ((iname == NULL) || (dev == NULL)) {
		usage_exit(argv[0], "-i file -d part");
	}

	if (!wait_action_idle(10)) {
		printf("System is busy\n");
		return 1;
	}
	set_action(ACT_WEB_UPGRADE);

	if ((f = fopen(iname, "r")) == NULL) {
		error = "Error opening input file";
		goto ERROR;
	}

	error = "File contains an invalid header";

	if (safe_fread(&sig, 1, sizeof(sig), f) != sizeof(sig)) {
		goto ERROR;
	}
	switch (sig) {
	case 0x47343557: // W54G	G, GL
	case 0x53343557: // W54S	GS
	case 0x73343557: // W54s	GS v4
	case 0x55343557: // W54U	SL
	case 0x31345257: // WR41	WRH54G
	case 0x4E303233: // 320N	WRT320N
	case 0x4E583233: // 32XN	E2000
	case 0x4E303136: // 610N	WRT610N v2
	case 0x4E583136: // 61XN	E3000
	case 0x30303945: // E900	E900
	case 0x30303145: // E100	E1000
	case 0x30323145: // E120	E1200v1
	case 0x32323145: // E122	E1200v2
	case 0x30353145: // E150	E1500
	case 0x30353531: // 1550	E1550
	case 0x3031304D: // M010	M10
	case 0x3032304D: // M020	M20
	case 0x3036314E: // N160	WRT160N
	case 0x42435745: // EWCB	WRT300N v1
	case 0x4E303133: // 310N	WRT310N v1/v2
//	case 0x32435745: // EWC2	WRT300N?
	case 0x3035314E: // N150	WRT150N
	case 0x58353245: // E25X	E2500
	case 0x30303233: // 3200	E3200
	case 0x30303234: // 4200	E4200
		if (safe_fread(((char *)&cth) + 4, 1, sizeof(cth) - 4, f) != (sizeof(cth) - 4)) {
			goto ERROR;
		}
		if (memcmp(cth.id, "U2ND", 4) != 0) {
			goto ERROR;
		}

		// trx should be next...
		if (safe_fread(&sig, 1, sizeof(sig), f) != sizeof(sig)) {
			goto ERROR;
		}
		break;
	case 0x5E24232A: // Netgear
		// header length is next
		if (safe_fread(&n, 1, sizeof(n), f) != sizeof(n)) {
			goto ERROR;
		}
		// skip the header - we can't use seek() for fifo, so read the rest of the header
		n = ntohl(n) - sizeof(sig) - sizeof(n);
		if ((buf = malloc(n + 1)) == NULL) {
			error = "Not enough memory";
			goto ERROR;
		}
		if (safe_fread(buf, 1, n, f) != n) {
			goto ERROR;
		}
		free(buf);
		buf = NULL;
		// trx should be next...
		if (safe_fread(&sig, 1, sizeof(sig), f) != sizeof(sig)) {
			goto ERROR;
		}
		break;
	case TRX_MAGIC:
		break;
#ifdef CONFIG_BCMWL5
	case TRX_MAGIC_F7D3301:
	case TRX_MAGIC_F7D3302:
	case TRX_MAGIC_F7D4302:
	case TRX_MAGIC_F5D8235V3:
	case TRX_MAGIC_QA:
		sig = TRX_MAGIC;
		break;
#endif
	default:
		// moto
		if (safe_fread(&sig, 1, sizeof(sig), f) != sizeof(sig)) {
			goto ERROR;
		}
		switch (sig) {
		case 0x50705710:	// WR850G
			// trx
			if (safe_fread(&sig, 1, sizeof(sig), f) != sizeof(sig)) {
				goto ERROR;
			}
			break;
		default:
			goto ERROR;
		}
		break;
	}

	if (sig != TRX_MAGIC) {
		goto ERROR;
	}
	if ((safe_fread(((char *)&trx) + 4, 1, sizeof(trx) - 4, f) != (sizeof(trx) - 4)) || (trx.len <= sizeof(trx))) {
		goto ERROR;
	}

	model = get_model();

	switch (model) {
#ifdef CONFIG_BCMWL5
	case MODEL_F7D3301:
		trx.magic = TRX_MAGIC_F7D3301;
		break;
	case MODEL_F7D3302:
		trx.magic = TRX_MAGIC_F7D3302;
		break;
	case MODEL_F7D4302:
		trx.magic = TRX_MAGIC_F7D4302;
		break;
	case MODEL_F5D8235v3:
		trx.magic = TRX_MAGIC_F5D8235V3;
		break;
#endif
	default:
		trx.magic = sig;
		break;
	}

	if (!crc_init()) {
		error = "Not enough memory";
		goto ERROR;
	}
	crc = crc_calc(0xFFFFFFFF, (char *)&trx.flag_version, sizeof(struct trx_header) - OFFSETOF(struct trx_header, flag_version));

	if (trx.flag_version & TRX_NO_HEADER) {
		trx.len -= sizeof(struct trx_header);
		_dprintf("don't write header\n");
	}

	_dprintf("trx len=%db 0x%x\n", trx.len, trx.len);

	if ((mf = mtd_open(dev, &mi)) < 0) {
		error = "Error opening MTD device";
		goto ERROR;
	}

	if (mi.erasesize < sizeof(struct trx_header)) {
		error = "Error obtaining MTD information";
		goto ERROR;
	}

	_dprintf("mtd size=%6x, erasesize=%6x\n", mi.size, mi.erasesize);

	total = ROUNDUP(trx.len, mi.erasesize);
	if (total > mi.size) {
		error = "File is too big to fit in MTD";
		goto ERROR;
	}

	sysinfo(&si);
	if ((si.freeram * si.mem_unit) > (total + (256 * 1024))) {
		ei.length = total;
	}
	else {
//		ei.length = ROUNDUP((si.freeram - (256 * 1024)), mi.erasesize);
		ei.length = mi.erasesize;
	}
	_dprintf("freeram=%ld ei.length=%d total=%u\n", si.freeram, ei.length, total);

	if ((buf = malloc(ei.length)) == NULL) {
		error = "Not enough memory";
		goto ERROR;
	}

#ifdef DEBUG_SIMULATE
	FILE *of;
	if ((of = fopen("/mnt/out.bin", "w")) == NULL) {
		error = "Error creating test file";
		goto ERROR;
	}
#endif

	if (trx.flag_version & TRX_NO_HEADER) {
		ofs = 0;
	}
	else {
		memcpy(buf, &trx, sizeof(trx));
		ofs = sizeof(trx);
	}
	_dprintf("trx.len=%ub 0x%x ofs=%ub 0x%x\n", trx.len, trx.len, ofs, ofs);

	error = NULL;

	for (ei.start = 0; ei.start < total; ei.start += ei.length) {
		n = MIN(ei.length, trx.len) - ofs;
		if (safe_fread(buf + ofs, 1, n, f) != n) {
			error = "Error reading file";
			break;
		}
		trx.len -= (n + ofs);

		crc = crc_calc(crc, buf + ofs, n);

		if (trx.len == 0) {
			_dprintf("crc=%8x  trx.crc=%8x\n", crc, trx.crc32);
			if (crc != trx.crc32) {
				error = "Image is corrupt";
				break;
			}
		}

		if (!web) {
			printf("Writing %x-%x\r", ei.start, (ei.start + ei.length) - 1);
		}

		_dprintf("ofs=%ub  n=%ub 0x%x  trx.len=%ub  ei.start=0x%x  ei.length=0x%x  mi.erasesize=0x%x\n",
			   ofs, n, n, trx.len, ei.start, ei.length, mi.erasesize);

		n += ofs;

		_dprintf(" erase start=%x len=%x\n", ei.start, ei.length);
		_dprintf(" write %x\n", n);

#ifdef DEBUG_SIMULATE
		if (fwrite(buf, 1, n, of) != n) {
			fclose(of);
			error = "Error writing to test file";
			break;
		}
#else
		ioctl(mf, MEMUNLOCK, &ei);
		if (ioctl(mf, MEMERASE, &ei) != 0 && model != MODEL_WNR3500LV2) {
			error = "Error erasing MTD block";
			break;
		} 
		if (write(mf, buf, n) != n) {
			error = "Error writing to MTD device";
			break;
		}
#endif
		ofs = 0;
	}

	// Netgear WNR3500L: write fake len and checksum at the end of mtd

	char *tmp;
	char imageInfo[8];

	switch (model) {
	case MODEL_WNR3500L:
	case MODEL_WNR2000v2:
		error = "Error writing fake Netgear crc";

		// Netgear CFE has the offset of the checksum hardcoded as
		// 0x78FFF8 on 8MB flash, and 0x38FFF8 on 4MB flash - in both
		// cases this is 8 last bytes in the block exactly 6 blocks to the end.
		// We rely on linux partition to be sized correctly by the kernel,
		// so the checksum area doesn't fall outside of the linux partition,
		// and doesn't override the rootfs.
		ofs = (mi.size > (4 *1024 * 1024) ? 0x78FFF8 : 0x38FFF8) - 0x040000;

		n   = 0x00000004;	// fake length - little endian
		crc = 0x02C0010E;	// fake crc - little endian
		memcpy(&imageInfo[0], (char *)&n,   4);
		memcpy(&imageInfo[4], (char *)&crc, 4);

		ei.start = (ofs / mi.erasesize) * mi.erasesize;
		ei.length = mi.erasesize;

		if (lseek(mf, ei.start, SEEK_SET) < 0)
			goto ERROR2;
		if (buf) free(buf);
		if (!(buf = malloc(mi.erasesize)))
			goto ERROR2;
		if (read(mf, buf, mi.erasesize) != mi.erasesize)
			goto ERROR2;
		if (lseek(mf, ei.start, SEEK_SET) < 0)
			goto ERROR2;

		tmp = buf + (ofs % mi.erasesize);
		memcpy(tmp, imageInfo, sizeof(imageInfo));

#ifdef DEBUG_SIMULATE
		if (fseek(of, ei.start, SEEK_SET) < 0)
			goto ERROR2;
		if (fwrite(buf, 1, mi.erasesize, of) != n)
			goto ERROR2;
#else
		ioctl(mf, MEMUNLOCK, &ei);
		if (ioctl(mf, MEMERASE, &ei) != 0)
			goto ERROR2;

		if (write(mf, buf, mi.erasesize) != mi.erasesize)
			goto ERROR2;
#endif

ERROR2:
		_dprintf("%s.\n",  error ? : "Write Netgear fake len/crc completed");
		// ignore crc write errors
		error = NULL;
		break;
	}

#ifdef DEBUG_SIMULATE
	fclose(of);
#endif

ERROR:
	if (buf) free(buf);
	if (mf >= 0) {
		// dummy read to ensure chip(s) are out of lock/suspend state
		read(mf, &n, sizeof(n));
		close(mf);
	}
	if (f) fclose(f);

	crc_done();

#ifdef DEBUG_SIMULATE
	set_action(ACT_IDLE);
#endif

	printf("%s\n",  error ? error : "Image successfully flashed");
	_dprintf("%s\n",  error ? error : "Image successfully flashed");
	return (error ? 1 : 0);
}
