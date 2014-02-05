/*
 * tc_core.c		TC core library.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 * Authors:	Alexey Kuznetsov, <kuznet@ms2.inr.ac.ru>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <fcntl.h>
#include <math.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include "tc_core.h"

#define	ATM_CELL_SIZE		53
#define	ATM_CELL_PAYLOAD	48

static __u32 t2us=1;
static __u32 us2t=1;
static double tick_in_usec = 1;

long tc_core_usec2tick(long usec)
{
	return usec*tick_in_usec;
}

long tc_core_tick2usec(long tick)
{
	return tick/tick_in_usec;
}

unsigned tc_calc_xmittime(unsigned rate, unsigned size)
{
	return tc_core_usec2tick(1000000*((double)size/rate));
}

/*
 * Calculate the ATM cell overhead.  ATM sends each packet in 48 byte
 * chunks, the last chunk being padded if necessary.  Each chunk carries
 * an additional 5 byte overhead - the ATM header.*/

static int tc_align_to_cells(int size) 
{
	int cells;

	cells = size / ATM_CELL_PAYLOAD;
	if (size % ATM_CELL_PAYLOAD != 0)
		cells++;
	return cells * ATM_CELL_SIZE;
}

/** The number this function calculates is subtle.  Ignore it and just believe
 * it works if you have a choice, otherwise ..
 *
 * If there we are calculating the ATM cell overhead the kernel calculations
 * will be out sometimes if the range of packet sizes spanned by one
 * rate table element crosses an ATM cell boundary.  Consider these three
 * senarios:
 *    (a) the packet is sent across the ATM link without addition
 *        overheads the kernel doesn't know about, and
 *    (b) a packet that has 1 byte of additional overhead the kernel
 *        doesn't know about.  Here
 *    (c) a packet that has 2 bytes of additional overhead the
 *        kernel doesn't know about.
 * The table below presents what happens.  Each row is for a single rate
 * table element.  The "Sizes" column shows what packet sizes the rate table
 * element will be used for.  This packet size includes the "unknown to
 * kernel" overhead, but does not include overhead incurred by breaking the
 * packet up into ATM cells. This ATM cell overhead consists of the 5 byte
 * header per ATM cell, plus the padding in the last cell.  The "ATM" column
 * shows how many bytes are actually sent across the ATM link, ie it does
 * include the ATM cell overhead.
 *
 *   RateTable Entry  Sizes(a) ATM(a)    Sizes(b) ATM(b)   Sizes(c) ATM(c)
 *      ratetable[0]    0..7    53        1..8     53        2..9    53
 *      ratetable[1]    8..15   53        9..16    53        2..17   53
 *      ratetable[2]   16..23   53       17..24    53       18..25   53
 *      ratetable[3]   24..31   53       25..32    53       26..33   53
 *      ratetable[4]   32..39   53       33..40    53       34..41   53
 *      ratetable[5]   40..47   53       41..48    53       42..49   53,106
 *      ratetable[6]   48..55   53,106   49..56   106       50..57  106
 *
 * For senario (a), the ratetable[6] entry covers two cases: one were a single
 * ATM cell is needed to transmit the data, and one where two ATM cells are
 * required.  It can't be right for both.  Unfortunately the error is large.
 * The same problem arises in senario (c) for ratetable[5].  The problem
 * doesn't happen for senario (b), because the boundary between rate table
 * entries happens to match the boundary between ATM cells.
 *
 * What we would like to do is ensure that ratetable boundaries always match
 * the ATM cells.  If we do this the error goes away.  The solution is to make
 * the kernel add a small bias to the packet size.  (Small because the bias
 * will always be smaller than cell_log.)  Adding this small bias will in
 * effect slide the ratetable along a bit, so the boundaries match.  The code
 * below calculates that bias.  Provided the MTU is less than 4092, doing
 * this can always eliminate the error.
 *
 * Old kernels won't add this bias, so they will have the error described above
 * in most cases.  In the worst case senario, considering all possible ATM cell
 * sizes (1..48), for 7 of these sizes the old kernel will calculate the rate
 * wrongly - ie, be out by 53 bytes.*/

static int tc_calc_cell_align(int atm_cell_tax, char overhead, int cell_log)
{
	int cell_size;

  	if (!atm_cell_tax)
	  	return 0;
	cell_size = 1 << cell_log;
	return (overhead + cell_size - 2) % cell_size - cell_size + 1;
}


/*
 * A constructor for a tc_ratespec.
 */
void tc_calc_ratespec(struct tc_ratespec* spec, __u32* rtab, unsigned bps,
	int cell_log, unsigned mtu, unsigned char mpu, int atm_cell_tax,
	char overhead)
{
	int i;

	if (mtu == 0)
		mtu = 2047;

/*    rtab[pkt_len>>cell_log] = pkt_xmit_time  */
	if (cell_log < 0) {
		cell_log = 0;
		while ((mtu>>cell_log) > 255)
			cell_log++;
	}
	for (i=0; i<256; i++) {
		/*
		* sz is the length of packet we will use for this ratetable
		* entry.  The time taken to send a packet of this length will
		* be used for all packet lengths this ratetable entry applies
		* to.  As underestimating how long it will take to transmit a
		* packet is a worse error than overestimating it, the longest
		* packet this rate table entry applies to is used.
		*/
		int sz = ((i+1)<<cell_log) - 1 + overhead;
		if (sz < mpu)
			sz = mpu;
		if (atm_cell_tax)
			sz = tc_align_to_cells(sz);
		rtab[i] = tc_calc_xmittime(bps, sz);
	}
	
	spec->cell_align = tc_calc_cell_align(atm_cell_tax, overhead, cell_log);
	spec->cell_log = cell_log;
	spec->feature = 0x8000 | (atm_cell_tax ? 1 : 0);
	spec->mpu = mpu | (unsigned)(overhead << 8);
	spec->rate = bps;
}

int tc_core_init()
{
	FILE *fp = fopen("/proc/net/psched", "r");

	if (fp == NULL)
		return -1;

	if (fscanf(fp, "%08x%08x", &t2us, &us2t) != 2) {
		fclose(fp);
		return -1;
	}
	fclose(fp);
	tick_in_usec = (double)t2us/us2t;
	return 0;
}
