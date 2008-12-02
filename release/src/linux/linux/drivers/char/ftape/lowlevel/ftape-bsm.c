/*
 *      Copyright (C) 1994-1996 Bas Laarhoven,
 *                (C) 1996-1997 Claus Heine.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2, or (at your option)
 any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; see the file COPYING.  If not, write to
 the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

 *
 * $Source: /home/cvsroot/wrt54g/src/linux/linux/drivers/char/ftape/lowlevel/ftape-bsm.c,v $
 * $Revision: 1.1.1.2 $
 * $Date: 2003/10/14 08:08:06 $
 *
 *      This file contains the bad-sector map handling code for
 *      the QIC-117 floppy tape driver for Linux.
 *      QIC-40, QIC-80, QIC-3010 and QIC-3020 maps are implemented.
 */

#include <linux/string.h>

#include <linux/ftape.h>
#include "../lowlevel/ftape-tracing.h"
#include "../lowlevel/ftape-bsm.h"
#include "../lowlevel/ftape-ctl.h"
#include "../lowlevel/ftape-rw.h"

/*      Global vars.
 */

/*      Local vars.
 */
static __u8 *bad_sector_map;
static SectorCount *bsm_hash_ptr; 

typedef enum {
	forward, backward
} mode_type;


/*   given buffer that contains a header segment, find the end of
 *   of the bsm list
 */
__u8 * ftape_find_end_of_bsm_list(__u8 * address)
{
	__u8 *ptr   = address + FT_HEADER_END; /* start of bsm list */
	__u8 *limit = address + FT_SEGMENT_SIZE;
	while (ptr + 2 < limit) {
		if (ptr[0] || ptr[1] || ptr[2]) {
			ptr += 3;
		} else {
			return ptr;
		}
	}
	return NULL;
}

static inline void put_sector(SectorCount *ptr, unsigned int sector)
{
	ptr->bytes[0] = sector & 0xff;
	sector >>= 8;
	ptr->bytes[1] = sector & 0xff;
	sector >>= 8;
	ptr->bytes[2] = sector & 0xff;
}

static inline unsigned int get_sector(SectorCount *ptr)
{
	unsigned int sector;

	sector  = ptr->bytes[0];
	sector += ptr->bytes[1] <<  8;
	sector += ptr->bytes[2] << 16;

	return sector;
}

static void bsm_debug_fake(void)
{
	/* for testing of bad sector handling at end of tape
	 */
	/*  Enable to test bad sector handling
	 */
	/*  Enable when testing multiple volume tar dumps.
	 */
	/*  Enable when testing bit positions in *_error_map
	 */
}

static void print_bad_sector_map(void)
{
	unsigned int good_sectors;
	unsigned int total_bad = 0;
	int i;
	TRACE_FUN(ft_t_flow);

	if (ft_format_code == fmt_big || 
	    ft_format_code == fmt_var || 
	    ft_format_code == fmt_1100ft) {
		SectorCount *ptr = (SectorCount *)bad_sector_map;
		unsigned int sector;

		while((sector = get_sector(ptr++)) != 0) {
			if ((ft_format_code == fmt_big || 
			     ft_format_code == fmt_var) &&
			    sector & 0x800000) {
				total_bad += FT_SECTORS_PER_SEGMENT - 3;
				TRACE(ft_t_noise, "bad segment at sector: %6d",
				      sector & 0x7fffff);
			} else {
				++total_bad;
				TRACE(ft_t_noise, "bad sector: %6d", sector);
			}
		}
		/*  Display old ftape's end-of-file marks
		 */
		while ((sector = get_unaligned(((__u16*)ptr)++)) != 0) {
			TRACE(ft_t_noise, "Old ftape eof mark: %4d/%2d",
			      sector, get_unaligned(((__u16*)ptr)++));
		}
	} else { /* fixed size format */
		for (i = ft_first_data_segment;
		     i < (int)(ft_segments_per_track * ft_tracks_per_tape); ++i) {
			SectorMap map = ((SectorMap *) bad_sector_map)[i];

			if (map) {
				TRACE(ft_t_noise,
				      "bsm for segment %4d: 0x%08x", i, (unsigned int)map);
				total_bad += ((map == EMPTY_SEGMENT)
					       ? FT_SECTORS_PER_SEGMENT - 3
					       : count_ones(map));
			}
		}
	}
	good_sectors =
		((ft_segments_per_track * ft_tracks_per_tape - ft_first_data_segment)
		 * (FT_SECTORS_PER_SEGMENT - 3)) - total_bad;
	TRACE(ft_t_info, "%d Kb usable on this tape", good_sectors);
	if (total_bad == 0) {
		TRACE(ft_t_info,
		      "WARNING: this tape has no bad blocks registered !");
	} else {
		TRACE(ft_t_info, "%d bad sectors", total_bad);
	}
	TRACE_EXIT;
}


void ftape_extract_bad_sector_map(__u8 * buffer)
{
	TRACE_FUN(ft_t_any);

	/*  Fill the bad sector map with the contents of buffer.
	 */
	if (ft_format_code == fmt_var || ft_format_code == fmt_big) {
		/* QIC-3010/3020 and wide QIC-80 tapes no longer have a failed
		 * sector log but use this area to extend the bad sector map.
		 */
		bad_sector_map = &buffer[FT_HEADER_END];
	} else {
		/* non-wide QIC-80 tapes have a failed sector log area that
		 * mustn't be included in the bad sector map.
		 */
		bad_sector_map = &buffer[FT_FSL + FT_FSL_SIZE];
	}
	if (ft_format_code == fmt_1100ft || 
	    ft_format_code == fmt_var    ||
	    ft_format_code == fmt_big) {
		bsm_hash_ptr = (SectorCount *)bad_sector_map;
	} else {
		bsm_hash_ptr = NULL;
	}
	bsm_debug_fake();
	if (TRACE_LEVEL >= ft_t_info) {
		print_bad_sector_map();
	}
	TRACE_EXIT;
}

static inline SectorMap cvt2map(unsigned int sector)
{
	return 1 << (((sector & 0x7fffff) - 1) % FT_SECTORS_PER_SEGMENT);
}

static inline int cvt2segment(unsigned int sector)
{
	return ((sector & 0x7fffff) - 1) / FT_SECTORS_PER_SEGMENT;
}

static int forward_seek_entry(int segment_id, 
			      SectorCount **ptr, 
			      SectorMap *map)
{
	unsigned int sector;
	int segment;

	do {
		sector = get_sector((*ptr)++);
		segment = cvt2segment(sector);
	} while (sector != 0 && segment < segment_id);
	(*ptr) --; /* point to first sector >= segment_id */
	/*  Get all sectors in segment_id
	 */
	if (sector == 0 || segment != segment_id) {
		*map = 0;
		return 0;
	} else if ((sector & 0x800000) &&
		   (ft_format_code == fmt_var || ft_format_code == fmt_big)) {
		*map = EMPTY_SEGMENT;
		return FT_SECTORS_PER_SEGMENT;
	} else {
		int count = 1;
		SectorCount *tmp_ptr = (*ptr) + 1;
		
		*map = cvt2map(sector);
		while ((sector = get_sector(tmp_ptr++)) != 0 &&
		       (segment = cvt2segment(sector)) == segment_id) {
			*map |= cvt2map(sector);
			++count;
		}
		return count;
	}
}

static int backwards_seek_entry(int segment_id,
				SectorCount **ptr,
				SectorMap *map)
{
	unsigned int sector;
	int segment; /* max unsigned int */

	if (*ptr <= (SectorCount *)bad_sector_map) {
		*map = 0;
		return 0;
	}
	do {
		sector  = get_sector(--(*ptr));
		segment = cvt2segment(sector);
	} while (*ptr > (SectorCount *)bad_sector_map && segment > segment_id);
	if (segment > segment_id) { /*  at start of list, no entry found */
		*map = 0;
		return 0;
	} else if (segment < segment_id) {
		/*  before smaller entry, adjust for overshoot */
		(*ptr) ++;
		*map = 0;
		return 0;
	} else if ((sector & 0x800000) &&
		   (ft_format_code == fmt_big || ft_format_code == fmt_var)) {
		*map = EMPTY_SEGMENT;
		return FT_SECTORS_PER_SEGMENT;
	} else { /*  get all sectors in segment_id */
		int count = 1;

		*map = cvt2map(sector);
		while(*ptr > (SectorCount *)bad_sector_map) {
			sector = get_sector(--(*ptr));
			segment = cvt2segment(sector);
			if (segment != segment_id) {
				break;
			}
			*map |= cvt2map(sector);
			++count;
		}
		if (segment < segment_id) {
			(*ptr) ++;
		}
		return count;
	}
}

void ftape_put_bad_sector_entry(int segment_id, SectorMap new_map)
{
	SectorCount *ptr = (SectorCount *)bad_sector_map;
	int count;
	int new_count;
	SectorMap map;
	TRACE_FUN(ft_t_any);

	if (ft_format_code == fmt_1100ft || 
	    ft_format_code == fmt_var || 
	    ft_format_code == fmt_big) {
		count = forward_seek_entry(segment_id, &ptr, &map);
		new_count = count_ones(new_map);
		/* If format code == 4 put empty segment instead of 32
		 * bad sectors.
		 */
		if (ft_format_code == fmt_var || ft_format_code == fmt_big) {
			if (new_count == FT_SECTORS_PER_SEGMENT) {
				new_count = 1;
			}
			if (count == FT_SECTORS_PER_SEGMENT) {
				count = 1;
			}
		}
		if (count != new_count) {
			/* insert (or delete if < 0) new_count - count
			 * entries.  Move trailing part of list
			 * including terminating 0.
			 */
			SectorCount *hi_ptr = ptr;

			do {
			} while (get_sector(hi_ptr++) != 0);
			/*  Note: ptr is of type byte *, and each bad sector
			 *  consumes 3 bytes.
			 */
			memmove(ptr + new_count, ptr + count,
				(size_t)(hi_ptr - (ptr + count))*sizeof(SectorCount));
		}
		TRACE(ft_t_noise, "putting map 0x%08x at %p, segment %d",
		      (unsigned int)new_map, ptr, segment_id);
		if (new_count == 1 && new_map == EMPTY_SEGMENT) {
			put_sector(ptr++, (0x800001 + 
					  segment_id * 
					  FT_SECTORS_PER_SEGMENT));
		} else {
			int i = 0;

			while (new_map) {
				if (new_map & 1) {
					put_sector(ptr++, 
						   1 + segment_id * 
						   FT_SECTORS_PER_SEGMENT + i);
				}
				++i;
				new_map >>= 1;
			}
		}
	} else {
		((SectorMap *) bad_sector_map)[segment_id] = new_map;
	}
	TRACE_EXIT;
}

SectorMap ftape_get_bad_sector_entry(int segment_id)
{
	if (ft_used_header_segment == -1) {
		/*  When reading header segment we'll need a blank map.
		 */
		return 0;
	} else if (bsm_hash_ptr != NULL) {
		/*  Invariants:
		 *    map - mask value returned on last call.
		 *    bsm_hash_ptr - points to first sector greater or equal to
		 *          first sector in last_referenced segment.
		 *    last_referenced - segment id used in the last call,
		 *                      sector and map belong to this id.
		 *  This code is designed for sequential access and retries.
		 *  For true random access it may have to be redesigned.
		 */
		static int last_reference = -1;
		static SectorMap map;

		if (segment_id > last_reference) {
			/*  Skip all sectors before segment_id
			 */
			forward_seek_entry(segment_id, &bsm_hash_ptr, &map);
		} else if (segment_id < last_reference) {
			/* Skip backwards until begin of buffer or
			 * first sector in segment_id 
			 */
			backwards_seek_entry(segment_id, &bsm_hash_ptr, &map);
		}		/* segment_id == last_reference : keep map */
		last_reference = segment_id;
		return map;
	} else {
		return ((SectorMap *) bad_sector_map)[segment_id];
	}
}

/*  This is simply here to prevent us from overwriting other kernel
 *  data. Writes will result in NULL Pointer dereference.
 */
void ftape_init_bsm(void)
{
	bad_sector_map = NULL;
	bsm_hash_ptr   = NULL;
}
