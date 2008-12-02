/*
 *  arch/mips/galileo/compressed/burner.c
 *
 *  By RidgeRun Inc (Leveraged from Galileo's main.c, misc.c, etc).
 *
 *  Burn image from ram to flash
 *  For use with Galileo EVB64120A MIPS eval board.
 */
#include <asm/types.h>
#include <asm/byteorder.h>
#include <asm/galileo-boards/evb64120A/eeprom_param.h>
#include <asm/galileo-boards/evb64120A/flashdrv.h>

#define IMAGEOFFSET 0x00300000

static void burn_image_from_memory(void);
static char *sprintf(char *buf, const char *fmt, ...);
static void printf(const char *fmt, ...);

unsigned int FlashSize;

/******************************
 Routine:
 Description:
 ******************************/
int main(void)
{
	printf("\n");
	printf("\n");
	printf("\n");
	printf("\n");
	printf("       +--------------------+\n");
	printf("       |                    |\n");
	printf("       | Flash Burn Utility |\n");
	printf("       |                    |\n");
	printf("       +--------------------+\n");
	printf("\n");
	printf("Please send your *.srec image to the parallel port\n");
	printf("\n");
	printf("Note: The *.srec image should be setup to\n");
	printf("      load into address 0xa0300000 where\n");
	printf("      it will then be transferred to flash\n");
	printf("\n");

	SET_REG_BITS(0x468, BIT20);	// Set Flash to be 16 bit wide
	FlashSize = flashInit(0xbf000000, 2, X16);

	galileo_dl();		// read in the users *.srec image.
	burn_image_from_memory();	// put it in flash.

	printf("\n");
	printf("\n");
	printf("+---------------+\n");
	printf("|     Done      |\n");
	printf("|(please reboot)|\n");
	printf("+---------------+\n");
	printf("\n");

	while (1) {
	}
	return 0;
}

/******************************
 Routine:
 Description:
 ******************************/
static void burn_image_from_memory(void)
{
	unsigned int count, delta, temp, temp1;
	unsigned int to_sector, last_sector;

	/* Find how many sectors needed to be erased */
	to_sector = flashInWhichSector(FlashSize - 4);	// skranz, modified.
	if (to_sector == 0xffffffff) {
		printf
		    ("Flash Burning Error - Flash too small - Cannot burn image\n");	// skranz, modified.
		return;
	}

	/* Which is the last sector */
	last_sector = flashInWhichSector(FlashSize - 4);
	delta = 0;
	printf("\nErasing first %d sectors\n", to_sector);
	for (count = 0; count < to_sector + 1; count++) {
		printf("Erasing sector %d\n", count);
		flashEraseSector(count);
	}
	printf("flash region size = %d\n", FlashSize);	// skranz, added
	printf("Sdram IMAGEOFFSET = %d\n", IMAGEOFFSET);	// skranz, added

	printf("Burning from Sdram to %d mark; full burn.\n", FlashSize);
	for (count = 0; count < (FlashSize - delta); count = count + 4) {
		flashWriteWord(count,
			       *(unsigned int *) ((count | NONE_CACHEABLE)
						  + IMAGEOFFSET));	// skranz, modified.
		temp = flashReadWord(count);
		temp1 = *(unsigned int *) ((count | NONE_CACHEABLE) + IMAGEOFFSET);	// skranz, modified.
		if (((unsigned int) temp) != ((unsigned int) temp1)) {
			printf
			    ("Burning error at address %X : flash(%X) sdram(%X)\n",
			     count, flashReadWord(count),
			     *(unsigned int *) ((count | NONE_CACHEABLE) + IMAGEOFFSET));	// skranz, modified.
			printf("Aborting Prematurally.\n");
			break;
		}
	}
	printf("Finished burning Image\n");
}

/******************************
 Routine:
 Description:
	Formats:
		%X	- 4 byte ASCII (8 hex digits)
		%x	- 2 byte ASCII (4 hex digits)
		%b	- 1 byte ASCII (2 hex digits)
		%d	- decimal (also %i)
		%c	- ASCII char
		%s	- ASCII string
		%I	- Internet address in x.x.x.x notation
 ******************************/
static char hex[] = "0123456789ABCDEF";
static char *do_printf(char *buf, const char *fmt, const int *dp)
{
	register char *p;
	char tmp[16];
	while (*fmt) {
		if (*fmt == '%') {	/* switch() uses more space */
			fmt++;

			if (*fmt == 'X') {
				const long *lp = (const long *) dp;
				register long h = *lp++;
				dp = (const int *) lp;
				*(buf++) = hex[(h >> 28) & 0x0F];
				*(buf++) = hex[(h >> 24) & 0x0F];
				*(buf++) = hex[(h >> 20) & 0x0F];
				*(buf++) = hex[(h >> 16) & 0x0F];
				*(buf++) = hex[(h >> 12) & 0x0F];
				*(buf++) = hex[(h >> 8) & 0x0F];
				*(buf++) = hex[(h >> 4) & 0x0F];
				*(buf++) = hex[h & 0x0F];
			}
			if (*fmt == 'x') {
				register int h = *(dp++);
				*(buf++) = hex[(h >> 12) & 0x0F];
				*(buf++) = hex[(h >> 8) & 0x0F];
				*(buf++) = hex[(h >> 4) & 0x0F];
				*(buf++) = hex[h & 0x0F];
			}
			if (*fmt == 'b') {
				register int h = *(dp++);
				*(buf++) = hex[(h >> 4) & 0x0F];
				*(buf++) = hex[h & 0x0F];
			}
			if ((*fmt == 'd') || (*fmt == 'i')) {
				register int dec = *(dp++);
				p = tmp;
				if (dec < 0) {
					*(buf++) = '-';
					dec = -dec;
				}
				do {
					*(p++) = '0' + (dec % 10);
					dec = dec / 10;
				} while (dec);
				while ((--p) >= tmp)
					*(buf++) = *p;
			}
			if (*fmt == 'I') {
				union {
					long l;
					unsigned char c[4];
				} u;
				const long *lp = (const long *) dp;
				u.l = *lp++;
				dp = (const int *) lp;
				buf = sprintf(buf, "%d.%d.%d.%d",
					      u.c[0], u.c[1], u.c[2],
					      u.c[3]);
			}
			if (*fmt == 'c')
				*(buf++) = *(dp++);
			if (*fmt == 's') {
				p = (char *) *dp++;
				while (*p)
					*(buf++) = *p++;
			}
		} else
			*(buf++) = *fmt;
		fmt++;
	}
	*buf = 0;
	return (buf);
}

/******************************
 Routine:
 Description:
 ******************************/
static char *sprintf(char *buf, const char *fmt, ...)
{
	return do_printf(buf, fmt, ((const int *) &fmt) + 1);
}

/******************************
 Routine:
 Description:
 ******************************/
void putchar(int c)
{
	if (c == '\n') {
		serial_putc('\r');
	}
	serial_putc(c);
}

/******************************
 Routine:
 Description:
 ******************************/
static void printf(const char *fmt, ...)
{
	char buf[256], *p;
	p = buf;
	do_printf(buf, fmt, ((const int *) &fmt) + 1);
	while (*p)
		putchar(*p++);
}
