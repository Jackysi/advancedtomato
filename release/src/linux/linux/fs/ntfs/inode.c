/*
 * inode.c
 *
 * Copyright (C) 1995-1999 Martin von Löwis
 * Copyright (C) 1996 Albert D. Cahalan
 * Copyright (C) 1996-1997 Régis Duchesne
 * Copyright (C) 1998 Joseph Malicki
 * Copyright (C) 1999 Steve Dodd
 * Copyright (C) 2000-2001 Anton Altaparmakov (AIA)
 */
#include "ntfstypes.h"
#include "ntfsendian.h"
#include "struct.h"
#include "inode.h"
#include <linux/errno.h>
#include "macros.h"
#include "attr.h"
#include "super.h"
#include "dir.h"
#include "support.h"
#include "util.h"
#include <linux/ntfs_fs.h>
#include <linux/smp_lock.h>

typedef struct {
	int recno;
	unsigned char *record;
} ntfs_mft_record;

typedef struct {
	int size;
	int count;
	ntfs_mft_record *records;
} ntfs_disk_inode;

static void ntfs_fill_mft_header(ntfs_u8 *mft, int rec_size, int seq_no,
		int links, int flags)
{
	int fixup_ofs = 0x2a;
	int fixup_cnt = rec_size / NTFS_SECTOR_SIZE + 1;
	int attr_ofs = (fixup_ofs + 2 * fixup_cnt + 7) & ~7;

	NTFS_PUTU32(mft + 0x00, 0x454c4946);	/* FILE */
	NTFS_PUTU16(mft + 0x04, fixup_ofs);	/* Offset to fixup. */
	NTFS_PUTU16(mft + 0x06, fixup_cnt);	/* Number of fixups. */
	NTFS_PUTU64(mft + 0x08, 0);		/* Logical sequence number. */
	NTFS_PUTU16(mft + 0x10, seq_no);	/* Sequence number. */
	NTFS_PUTU16(mft + 0x12, links);		/* Hard link count. */
	NTFS_PUTU16(mft + 0x14, attr_ofs);	/* Offset to attributes. */
	NTFS_PUTU16(mft + 0x16, flags);		/* Flags: 1 = In use,
							  2 = Directory. */
	NTFS_PUTU32(mft + 0x18, attr_ofs + 8);	/* Bytes in use. */
	NTFS_PUTU32(mft + 0x1c, rec_size);	/* Total allocated size. */
	NTFS_PUTU64(mft + 0x20, 0);		/* Base mft record. */
	NTFS_PUTU16(mft + 0x28, 0);		/* Next attr instance. */
	NTFS_PUTU16(mft + fixup_ofs, 1);	/* Fixup word. */
	NTFS_PUTU32(mft + attr_ofs, (__u32)-1);	/* End of attributes marker. */
}

ntfs_attribute *ntfs_find_attr(ntfs_inode *ino, int type, char *name)
{
	int i;
	
	if (!ino) {
		ntfs_error("ntfs_find_attr: NO INODE!\n");
		return 0;
	}
	for (i = 0; i < ino->attr_count; i++) {
		if (type < ino->attrs[i].type)
			return 0;
		if (type == ino->attrs[i].type) {
			if (!name) {
				if (!ino->attrs[i].name)
					return ino->attrs + i;
			} else if (ino->attrs[i].name &&
				   !ntfs_ua_strncmp(ino->attrs[i].name, name,
						    strlen(name)))
				return ino->attrs + i;
		}
	}
	return 0;
}

static int ntfs_insert_mft_attributes(ntfs_inode* ino, char *mft, int mftno)
{
	int i, error, type, len, present = 0;
	char *it;

	/* Check for duplicate extension record. */
	for(i = 0; i < ino->record_count; i++)
		if (ino->records[i] == mftno) {
			if (i)
				return 0;
			present = 1;
			break;
		}
	if (!present) {
		/* (re-)allocate space if necessary. */
		if (ino->record_count % 8 == 0)	{
			int *new;

			new = ntfs_malloc((ino->record_count + 8) *
								sizeof(int));
			if (!new)
				return -ENOMEM;
			if (ino->records) {
				for (i = 0; i < ino->record_count; i++)
					new[i] = ino->records[i];
				ntfs_free(ino->records);
			}
			ino->records = new;
		}
		ino->records[ino->record_count] = mftno;
		ino->record_count++;
	}
	it = mft + NTFS_GETU16(mft + 0x14); /* mft->attrs_offset */
	do {
		type = NTFS_GETU32(it);
		len = NTFS_GETU32(it + 4);
		if (type != -1) {
			error = ntfs_insert_attribute(ino, it);
			if (error)
				return error;
		}
		/* If we have just processed the attribute list and this is
		 * the first time we are parsing this (base) mft record then we
		 * are done so that the attribute list gets parsed before the
		 * entries in the base mft record. Otherwise we run into
		 * problems with encountering attributes out of order and when
		 * this happens with different attribute extents we die. )-:
		 * This way we are ok as the attribute list is always sorted
		 * fully and correctly. (-: */
		if (type == 0x20 && !present)
			return 0;
		it += len;
	} while (type != -1); /* Attribute listing ends with type -1. */
	return 0;
}

static int ntfs_insert_mft_attribute(ntfs_inode* ino, int mftno,
		ntfs_u8 *attr)
{
	int i, error, present = 0;

	/* Check for duplicate extension record. */
	for(i = 0; i < ino->record_count; i++)
		if (ino->records[i] == mftno) {
			present = 1;
			break;
		}
	if (!present) {
		/* (re-)allocate space if necessary. */
		if (ino->record_count % 8 == 0)	{
			int *new;

			new = ntfs_malloc((ino->record_count + 8) *
								sizeof(int));
			if (!new)
				return -ENOMEM;
			if (ino->records) {
				for (i = 0; i < ino->record_count; i++)
					new[i] = ino->records[i];
				ntfs_free(ino->records);
			}
			ino->records = new;
		}
		ino->records[ino->record_count] = mftno;
		ino->record_count++;
	}
	if (NTFS_GETU32(attr) == -1) {
		ntfs_debug(DEBUG_FILE3, "ntfs_insert_mft_attribute: attribute "
				"type is -1.\n");
		return 0;
	}
	error = ntfs_insert_attribute(ino, attr);
	if (error)
		return error;
	return 0;
}

/* Read and insert all the attributes of an 'attribute list' attribute.
 * Return the number of remaining bytes in *plen. */
static int parse_attributes(ntfs_inode *ino, ntfs_u8 *alist, int *plen)
{
	ntfs_u8 *mft, *attr;
	int mftno, l, error;
	int last_mft = -1;
	int len = *plen;
	int tries = 0;
	
	if (!ino->attr) {
		ntfs_error("parse_attributes: called on inode 0x%x without a "
				"loaded base mft record.\n", ino->i_number);
		return -EINVAL;
	}
	mft = ntfs_malloc(ino->vol->mft_record_size);
	if (!mft)
		return -ENOMEM;
	while (len > 8)	{
		l = NTFS_GETU16(alist + 4);
		if (l > len)
			break;
	        /* Process an attribute description. */
		mftno = NTFS_GETU32(alist + 0x10); 
		if (mftno != ino->i_number && mftno != last_mft) {
continue_after_loading_mft_data:
			last_mft = mftno;
			error = ntfs_read_mft_record(ino->vol, mftno, mft);
			if (error) {
				if (error == -EINVAL && !tries)
					goto force_load_mft_data;
failed_reading_mft_data:
				ntfs_debug(DEBUG_FILE3, "parse_attributes: "
					"ntfs_read_mft_record(mftno = 0x%x) "
					"failed\n", mftno);
				ntfs_free(mft);
				return error;
			}
		}
		attr = ntfs_find_attr_in_mft_rec(
				ino->vol,		/* ntfs volume */
				mftno == ino->i_number ?/* mft record is: */
					ino->attr:	/*   base record */
					mft,		/*   extension record */
				NTFS_GETU32(alist + 0),	/* type */
				(wchar_t*)(alist + alist[7]),	/* name */
				alist[6], 		/* name length */
				1,			/* ignore case */
				NTFS_GETU16(alist + 24)	/* instance number */
				);
		if (!attr) {
			ntfs_error("parse_attributes: mft records 0x%x and/or "
				       "0x%x corrupt!\n", ino->i_number, mftno);
			ntfs_free(mft);
			return -EINVAL; 
		}
		error = ntfs_insert_mft_attribute(ino, mftno, attr);
		if (error) {
			ntfs_debug(DEBUG_FILE3, "parse_attributes: "
				"ntfs_insert_mft_attribute(mftno 0x%x, "
				"attribute type 0x%x) failed\n", mftno,
				NTFS_GETU32(alist + 0));
			ntfs_free(mft);
			return error;
		}
		len -= l;
		alist += l;
	}
	ntfs_free(mft);
	*plen = len;
	return 0;
force_load_mft_data:
{
	ntfs_u8 *mft2, *attr2;
	int mftno2;
	int last_mft2 = last_mft;
	int len2 = len;
	int error2;
	int found2 = 0;
	ntfs_u8 *alist2 = alist;
	mft2 = ntfs_malloc(ino->vol->mft_record_size);
	if (!mft2) {
		ntfs_free(mft);
		return -ENOMEM;
	}
	ntfs_memcpy(mft2, mft, ino->vol->mft_record_size);
	while (len2 > 8) {
		l = NTFS_GETU16(alist2 + 4);
		if (l > len2)
			break;
		if (NTFS_GETU32(alist2 + 0x0) < ino->vol->at_data) {
			len2 -= l;
			alist2 += l;
			continue;
		}
		if (NTFS_GETU32(alist2 + 0x0) > ino->vol->at_data) {
			if (found2)
				break;
			/* Uh-oh! It really isn't there! */
			ntfs_error("Either the $MFT is corrupt or, equally "
					"likely, the $MFT is too complex for "
					"the current driver to handle. Please "
					"email the ntfs maintainer that you "
					"saw this message. Thank you.\n");
			goto failed_reading_mft_data;
		}
	        /* Process attribute description. */
		mftno2 = NTFS_GETU32(alist2 + 0x10); 
		if (mftno2 != ino->i_number && mftno2 != last_mft2) {
			last_mft2 = mftno2;
			error2 = ntfs_read_mft_record(ino->vol, mftno2, mft2);
			if (error2) {
				ntfs_debug(DEBUG_FILE3, "parse_attributes: "
					"ntfs_read_mft_record(mftno2 = 0x%x) "
					"failed\n", mftno2);
				ntfs_free(mft2);
				goto failed_reading_mft_data;
			}
		}
		attr2 = ntfs_find_attr_in_mft_rec(
				ino->vol,		 /* ntfs volume */
				mftno2 == ino->i_number ?/* mft record is: */
					ino->attr:	 /*  base record */
					mft2,		 /*  extension record */
				NTFS_GETU32(alist2 + 0),	/* type */
				(wchar_t*)(alist2 + alist2[7]),	/* name */
				alist2[6], 		 /* name length */
				1,			 /* ignore case */
				NTFS_GETU16(alist2 + 24) /* instance number */
				);
		if (!attr2) {
			ntfs_error("parse_attributes: mft records 0x%x and/or "
				       "0x%x corrupt!\n", ino->i_number,
				       mftno2);
			ntfs_free(mft2);
			goto failed_reading_mft_data;
		}
		error2 = ntfs_insert_mft_attribute(ino, mftno2, attr2);
		if (error2) {
			ntfs_debug(DEBUG_FILE3, "parse_attributes: "
				"ntfs_insert_mft_attribute(mftno2 0x%x, "
				"attribute2 type 0x%x) failed\n", mftno2,
				NTFS_GETU32(alist2 + 0));
			ntfs_free(mft2);
			goto failed_reading_mft_data;
		}
		len2 -= l;
		alist2 += l;
		found2 = 1;
	}
	ntfs_free(mft2);
	tries = 1;
	goto continue_after_loading_mft_data;
}
}

static void ntfs_load_attributes(ntfs_inode *ino)
{
	ntfs_attribute *alist;
	int datasize;
	int offset, len, delta;
	char *buf;
	ntfs_volume *vol = ino->vol;
	
	ntfs_debug(DEBUG_FILE2, "load_attributes 0x%x 1\n", ino->i_number);
	if (ntfs_insert_mft_attributes(ino, ino->attr, ino->i_number))
		return;
	ntfs_debug(DEBUG_FILE2, "load_attributes 0x%x 2\n", ino->i_number);
	alist = ntfs_find_attr(ino, vol->at_attribute_list, 0);
	ntfs_debug(DEBUG_FILE2, "load_attributes 0x%x 3\n", ino->i_number);
	if (!alist)
		return;
	ntfs_debug(DEBUG_FILE2, "load_attributes 0x%x 4\n", ino->i_number);
	datasize = alist->size;
	ntfs_debug(DEBUG_FILE2, "load_attributes 0x%x: alist->size = 0x%x\n",
			ino->i_number, alist->size);
	if (alist->resident) {
		parse_attributes(ino, alist->d.data, &datasize);
		return;
	}
	ntfs_debug(DEBUG_FILE2, "load_attributes 0x%x 5\n", ino->i_number);
	buf = ntfs_malloc(1024);
	if (!buf)    
		return;
	delta = 0;
	for (offset = 0; datasize; datasize -= len, offset += len) {
		ntfs_io io;
		
		io.fn_put = ntfs_put;
		io.fn_get = 0;
		io.param = buf + delta;
		len = 1024 - delta;
		if (len > datasize)
			len = datasize;
		ntfs_debug(DEBUG_FILE2, "load_attributes 0x%x: len = %i\n",
						ino->i_number, len);
		ntfs_debug(DEBUG_FILE2, "load_attributes 0x%x: delta = %i\n",
						ino->i_number, delta);
		io.size = len;
		if (ntfs_read_attr(ino, vol->at_attribute_list, 0, offset,
				   &io))
			ntfs_error("error in load_attributes\n");
		delta += len;
		ntfs_debug(DEBUG_FILE2, "load_attributes 0x%x: after += len, "
				"delta = %i\n", ino->i_number, delta);
		parse_attributes(ino, buf, &delta);
		ntfs_debug(DEBUG_FILE2, "load_attributes 0x%x: after "
				"parse_attr, delta = %i\n", ino->i_number,
				delta);
		if (delta)
			/* Move remaining bytes to buffer start. */
			ntfs_memmove(buf, buf + len - delta, delta);
	}
	ntfs_debug(DEBUG_FILE2, "load_attributes 0x%x 6\n", ino->i_number);
	ntfs_free(buf);
}
	
int ntfs_init_inode(ntfs_inode *ino, ntfs_volume *vol, int inum)
{
	char *buf;
	int error;

	ntfs_debug(DEBUG_FILE1, "Initializing inode 0x%x\n", inum);
	ino->i_number = inum;
	ino->vol = vol;
	ino->attr = buf = ntfs_malloc(vol->mft_record_size);
	if (!buf)
		return -ENOMEM;
	error = ntfs_read_mft_record(vol, inum, ino->attr);
	if (error) {
		ntfs_debug(DEBUG_OTHER, "Init inode: 0x%x failed\n", inum);
		return error;
	}
	ntfs_debug(DEBUG_FILE2, "Init inode: got mft 0x%x\n", inum);
	ino->sequence_number = NTFS_GETU16(buf + 0x10);
	ino->attr_count = 0;
	ino->record_count = 0;
	ino->records = 0;
	ino->attrs = 0;
	ntfs_load_attributes(ino);
	ntfs_debug(DEBUG_FILE2, "Init inode: done 0x%x\n", inum);
	return 0;
}

void ntfs_clear_inode(ntfs_inode *ino)
{
	int i;
	if (!ino->attr) {
		ntfs_error("ntfs_clear_inode: double free\n");
		return;
	}
	ntfs_free(ino->attr);
	ino->attr = 0;
	ntfs_free(ino->records);
	ino->records = 0;
	for (i = 0; i < ino->attr_count; i++) {
		if (ino->attrs[i].name)
			ntfs_free(ino->attrs[i].name);
		if (ino->attrs[i].resident) {
			if (ino->attrs[i].d.data)
				ntfs_free(ino->attrs[i].d.data);
		} else {
			if (ino->attrs[i].d.r.runlist)
				ntfs_vfree(ino->attrs[i].d.r.runlist);
		}
	}
	ntfs_free(ino->attrs);
	ino->attrs = 0;
}

/* Check and fixup a MFT record. */
int ntfs_check_mft_record(ntfs_volume *vol, char *record)
{
	return ntfs_fixup_record(record, "FILE", vol->mft_record_size);
}

/* Return (in result) the value indicating the next available attribute 
 * chunk number. Works for inodes w/o extension records only. */
int ntfs_allocate_attr_number(ntfs_inode *ino, int *result)
{
	if (ino->record_count != 1)
		return -EOPNOTSUPP;
	*result = NTFS_GETU16(ino->attr + 0x28);
	NTFS_PUTU16(ino->attr + 0x28, (*result) + 1);
	return 0;
}

/* Find the location of an attribute in the inode. A name of NULL indicates
 * unnamed attributes. Return pointer to attribute or NULL if not found. */
char *ntfs_get_attr(ntfs_inode *ino, int attr, char *name)
{
	/* Location of first attribute. */
	char *it = ino->attr + NTFS_GETU16(ino->attr + 0x14);
	int type;
	int len;
	
	/* Only check for magic DWORD here, fixup should have happened before.*/
	if (!IS_MFT_RECORD(ino->attr))
		return 0;
	do {
		type = NTFS_GETU32(it);
		len = NTFS_GETU16(it + 4);
		/* We found the attribute type. Is the name correct, too? */
		if (type == attr) {
			int namelen = NTFS_GETU8(it + 9);
			char *name_it, *n = name;
			/* Match given name and attribute name if present.
			   Make sure attribute name is Unicode. */
			if (!name) {
				goto check_namelen;
			} else if (namelen) {
				for (name_it = it + NTFS_GETU16(it + 10);
				     namelen; n++, name_it += 2, namelen--)
					if (*name_it != *n || name_it[1])
						break;
check_namelen:
				if (!namelen)
					break;
			}
		}
		it += len;
	} while (type != -1); /* List of attributes ends with type -1. */
	if (type == -1)
		return 0;
	return it;
}

__s64 ntfs_get_attr_size(ntfs_inode *ino, int type, char *name)
{
	ntfs_attribute *attr = ntfs_find_attr(ino, type, name);
	if (!attr)
		return 0;
	return
		attr->size;
}
	
int ntfs_attr_is_resident(ntfs_inode *ino, int type, char *name)
{
	ntfs_attribute *attr = ntfs_find_attr(ino, type, name);
	if (!attr)
		return 0;
	return attr->resident;
}
	
/*
 * A run is coded as a type indicator, an unsigned length, and a signed cluster
 * offset.
 * . To save space, length and offset are fields of variable length. The low
 *   nibble of the type indicates the width of the length :), the high nibble
 *   the width of the offset.
 * . The first offset is relative to cluster 0, later offsets are relative to
 *   the previous cluster.
 *
 * This function decodes a run. Length is an output parameter, data and cluster
 * are in/out parameters.
 */
int ntfs_decompress_run(unsigned char **data, int *length, 
			ntfs_cluster_t *cluster, int *ctype)
{
	unsigned char type = *(*data)++;
	*ctype = 0;
	switch (type & 0xF) {
	case 1: 
		*length = NTFS_GETS8(*data);
		break;
	case 2: 
		*length = NTFS_GETS16(*data);
		break;
	case 3: 
		*length = NTFS_GETS24(*data);
		break;
        case 4: 
		*length = NTFS_GETS32(*data);
		break;
        	/* Note: cases 5-8 are probably pointless to code, since how
		 * many runs > 4GB of length are there? At the most, cases 5
		 * and 6 are probably necessary, and would also require making
		 * length 64-bit throughout. */
	default:
		ntfs_error("Can't decode run type field 0x%x\n", type);
		return -1;
	}
//	ntfs_debug(DEBUG_FILE3, "ntfs_decompress_run: length = 0x%x\n",*length);
	if (*length < 0)
	{
		ntfs_error("Negative run length decoded\n");
		return -1;
	}
	*data += (type & 0xF);
	switch (type & 0xF0) {
	case 0:
		*ctype = 2;
		break;
	case 0x10:
		*cluster += NTFS_GETS8(*data);
		break;
	case 0x20:
		*cluster += NTFS_GETS16(*data);
		break;
	case 0x30:
		*cluster += NTFS_GETS24(*data);
		break;
	case 0x40:
		*cluster += NTFS_GETS32(*data);
		break;
	default:
		ntfs_error("Can't decode run type field 0x%x\n", type);
		return -1;
	}
//	ntfs_debug(DEBUG_FILE3, "ntfs_decompress_run: cluster = 0x%x\n",
//								*cluster);
	*data += (type >> 4);
	return 0;
}

static void dump_runlist(const ntfs_runlist *rl, const int rlen);

/* Reads l bytes of the attribute (attr, name) of ino starting at offset on
 * vol into buf. Returns the number of bytes read in the ntfs_io struct.
 * Returns 0 on success, errno on failure */
int ntfs_readwrite_attr(ntfs_inode *ino, ntfs_attribute *attr, __s64 offset,
		ntfs_io *dest)
{
	int rnum, s_vcn, error, clustersizebits;
	ntfs_cluster_t cluster, s_cluster, vcn, len;
	__s64 l, chunk, copied;

	ntfs_debug(DEBUG_FILE3, __FUNCTION__ "(): %s 0x%x bytes at offset "
			"0x%Lx %s inode 0x%x, attr type 0x%x.\n",
			dest->do_read ? "Read" : "Write", dest->size, offset,
			dest->do_read ? "from" : "to", ino->i_number,
			attr->type);
	l = dest->size;
	if (l == 0)
		return 0;
	if (dest->do_read) {
		/* If read _starts_ beyond end of stream, return nothing. */
		if (offset >= attr->size) {
			dest->size = 0;
			return 0;
		}
		/* If read _extends_ beyond end of stream, return as much
		 * initialised data as we have. */
		if (offset + l >= attr->size)
			l = dest->size = attr->size - offset;
	} else {
		/*
		 * If write extends beyond _allocated_ size, extend attribute,
		 * updating attr->allocated and attr->size in the process. (AIA)
		 */
		if ((!attr->resident && offset + l > attr->allocated) ||
				(attr->resident && offset + l > attr->size)) {
			error = ntfs_resize_attr(ino, attr, offset + l);
			if (error)
				return error;
		}
		if (!attr->resident) {
			/* Has amount of data increased? */
			if (offset + l > attr->size)
				attr->size = offset + l;
			/* Has amount of initialised data increased? */
			if (offset + l > attr->initialized) {
				attr->initialized = offset + l;
			}
		}
	}
	if (attr->resident) {
		if (dest->do_read)
			dest->fn_put(dest, (ntfs_u8*)attr->d.data + offset, l);
		else
			dest->fn_get((ntfs_u8*)attr->d.data + offset, dest, l);
		dest->size = l;
		return 0;
	}
	if (dest->do_read) {
		/* Read uninitialized data. */
		if (offset >= attr->initialized)
			return ntfs_read_zero(dest, l);
		if (offset + l > attr->initialized) {
			dest->size = chunk = attr->initialized - offset;
			error = ntfs_readwrite_attr(ino, attr, offset, dest);
			if (error || (dest->size != chunk && (error = -EIO, 1)))
				return error;
			dest->size += l - chunk;
			return ntfs_read_zero(dest, l - chunk);
		}
		if (attr->flags & ATTR_IS_COMPRESSED)
			return ntfs_read_compressed(ino, attr, offset, dest);
	} else {
		if (attr->flags & ATTR_IS_COMPRESSED)
			return ntfs_write_compressed(ino, attr, offset, dest);
	}
	vcn = 0;
	clustersizebits = ino->vol->cluster_size_bits;
	s_vcn = offset >> clustersizebits;
	for (rnum = 0; rnum < attr->d.r.len &&
			vcn + attr->d.r.runlist[rnum].len <= s_vcn; rnum++)
		vcn += attr->d.r.runlist[rnum].len;
	if (rnum == attr->d.r.len) {
		ntfs_debug(DEBUG_FILE3, __FUNCTION__ "(): EOPNOTSUPP: "
			"inode = 0x%x, rnum = %i, offset = 0x%Lx, vcn = 0x%x, "
			"s_vcn = 0x%x.\n", ino->i_number, rnum, offset, vcn,
			s_vcn);
		dump_runlist(attr->d.r.runlist, attr->d.r.len);
		return -EOPNOTSUPP;
	}
	copied = 0;
	while (l) {
		s_vcn = offset >> clustersizebits;
		cluster = attr->d.r.runlist[rnum].lcn;
		len = attr->d.r.runlist[rnum].len;
		s_cluster = cluster + s_vcn - vcn;
		chunk = ((__s64)(vcn + len) << clustersizebits) - offset;
		if (chunk > l)
			chunk = l;
		dest->size = chunk;
		error = ntfs_getput_clusters(ino->vol, s_cluster, offset -
				((__s64)s_vcn << clustersizebits), dest);
		if (error) {
			ntfs_error("Read/write error.\n");
			dest->size = copied;
			return error;
		}
		l -= chunk;
		copied += chunk;
		offset += chunk;
		if (l && offset >= ((__s64)(vcn + len) << clustersizebits)) {
			rnum++;
			vcn += len;
			cluster = attr->d.r.runlist[rnum].lcn;
			len = attr->d.r.runlist[rnum].len;
		}
	}
	dest->size = copied;
	return 0;
}

int ntfs_read_attr(ntfs_inode *ino, int type, char *name, __s64 offset,
		   ntfs_io *buf)
{
	ntfs_attribute *attr;

	buf->do_read = 1;
	attr = ntfs_find_attr(ino, type, name);
	if (!attr) {
		ntfs_debug(DEBUG_FILE3, __FUNCTION__ "(): attr 0x%x not found "
				"in inode 0x%x\n", type, ino->i_number);
		return -EINVAL;
	}
	return ntfs_readwrite_attr(ino, attr, offset, buf);
}

int ntfs_write_attr(ntfs_inode *ino, int type, char *name, __s64 offset,
		    ntfs_io *buf)
{
	ntfs_attribute *attr;
	
	buf->do_read = 0;
	attr = ntfs_find_attr(ino, type, name);
	if (!attr) {
		ntfs_debug(DEBUG_FILE3, __FUNCTION__ "(): attr 0x%x not found "
				"in inode 0x%x\n", type, ino->i_number);
		return -EINVAL;
	}
	return ntfs_readwrite_attr(ino, attr, offset, buf);
}

/* -2 = error, -1 = hole, >= 0 means real disk cluster (lcn). */
int ntfs_vcn_to_lcn(ntfs_inode *ino, int vcn)
{
	int rnum;
	ntfs_attribute *data;
	
	data = ntfs_find_attr(ino, ino->vol->at_data, 0);
	if (!data || data->resident || data->flags & (ATTR_IS_COMPRESSED |
			ATTR_IS_ENCRYPTED))
		return -2;
	if (data->size <= (__s64)vcn << ino->vol->cluster_size_bits)
		return -2;
	if (data->initialized <= (__s64)vcn << ino->vol->cluster_size_bits)
		return -1;
	for (rnum = 0; rnum < data->d.r.len &&
			vcn >= data->d.r.runlist[rnum].len; rnum++)
		vcn -= data->d.r.runlist[rnum].len;
	if (data->d.r.runlist[rnum].lcn >= 0)
		return data->d.r.runlist[rnum].lcn + vcn;
	return data->d.r.runlist[rnum].lcn + vcn;
}

static int allocate_store(ntfs_volume *vol, ntfs_disk_inode *store, int count)
{
	int i;
	
	if (store->count > count)
		return 0;
	if (store->size < count) {
		ntfs_mft_record *n = ntfs_malloc((count + 4) * 
						 sizeof(ntfs_mft_record));
		if (!n)
			return -ENOMEM;
		if (store->size) {
			for (i = 0; i < store->size; i++)
				n[i] = store->records[i];
			ntfs_free(store->records);
		}
		store->size = count + 4;
		store->records = n;
	}
	for (i = store->count; i < count; i++) {
		store->records[i].record = ntfs_malloc(vol->mft_record_size);
		if (!store->records[i].record)
			return -ENOMEM;
		store->count++;
	}
	return 0;
}

static void deallocate_store(ntfs_disk_inode* store)
{
	int i;
	
	for (i = 0; i < store->count; i++)
		ntfs_free(store->records[i].record);
	ntfs_free(store->records);
	store->count = store->size = 0;
	store->records = 0;
}

/**
 * layout_runs - compress runlist into mapping pairs array
 * @attr:	attribute containing the runlist to compress
 * @rec:	destination buffer to hold the mapping pairs array
 * @offs:	current position in @rec (in/out variable)
 * @size:	size of the buffer @rec
 *
 * layout_runs walks the runlist in @attr, compresses it and writes it out the
 * resulting mapping pairs array into @rec (up to a maximum of @size bytes are
 * written). On entry @offs is the offset in @rec at which to begin writing the
 * mapping pairs array. On exit, it contains the offset in @rec of the first
 * byte after the end of the mapping pairs array.
 */
static int layout_runs(ntfs_attribute *attr, char *rec, int *offs, int size)
{
	int i, len, offset, coffs;
	/* ntfs_cluster_t MUST be signed! (AIA) */
	ntfs_cluster_t cluster, rclus;
	ntfs_runlist *rl = attr->d.r.runlist;
	cluster = 0;
	offset = *offs;
	for (i = 0; i < attr->d.r.len; i++) {
		/*
		 * We cheat with this check on the basis that lcn will never
		 * be less than -1 and the lcn delta will fit in signed
		 * 32-bits (ntfs_cluster_t). (AIA)
		 */
		if (rl[i].lcn < (ntfs_cluster_t)-1) {
			ntfs_error("layout_runs() encountered an out of bounds "
					"cluster delta, lcn = %i.\n",
					rl[i].lcn);
			return -ERANGE;
		}
		rclus = rl[i].lcn - cluster;
		len = rl[i].len;
		rec[offset] = 0;
 		if (offset + 9 > size)
			return -E2BIG; /* It might still fit, but this
					* simplifies testing. */
		/*
		 * Run length is stored as signed number, so deal with it
		 * properly, i.e. observe that a negative number will have all
		 * its most significant bits set to 1 but we don't store that
		 * in the mapping pairs array. We store the smallest type of
		 * negative number required, thus in the first if we check
		 * whether len fits inside a signed byte and if so we store it
		 * as such, the next ifs check for a signed short, then a signed
		 * 24-bit and finally the full blown signed 32-bit. Same goes
		 * for rlus below. (AIA)
		 */
		if (len >= -0x80 && len <= 0x7f) {
			NTFS_PUTU8(rec + offset + 1, len & 0xff);
			coffs = 1;
 		} else if (len >= -0x8000 && len <= 0x7fff) {
			NTFS_PUTU16(rec + offset + 1, len & 0xffff);
			coffs = 2;
 		} else if (len >= -0x800000 && len <= 0x7fffff) {
			NTFS_PUTU24(rec + offset + 1, len & 0xffffff);
			coffs = 3;
		} else /* if (len >= -0x80000000LL && len <= 0x7fffffff */ {
			NTFS_PUTU32(rec + offset + 1, len);
			coffs = 4;
		} 
		*(rec + offset) |= coffs++;
		if (rl[i].lcn == (ntfs_cluster_t)-1) /* Compressed run. */
			/* Nothing */;
		else if (rclus >= -0x80 && rclus <= 0x7f) {
			*(rec + offset) |= 0x10;
			NTFS_PUTS8(rec + offset + coffs, rclus & 0xff);
			coffs += 1;
		} else if (rclus >= -0x8000 && rclus <= 0x7fff) {
			*(rec + offset) |= 0x20;
			NTFS_PUTS16(rec + offset + coffs, rclus & 0xffff);
			coffs += 2;
		} else if (rclus >= -0x800000 && rclus <= 0x7fffff) {
			*(rec + offset) |= 0x30;
			NTFS_PUTS24(rec + offset + coffs, rclus & 0xffffff);
			coffs += 3;
		} else /* if (rclus >= -0x80000000LL && rclus <= 0x7fffffff)*/ {
			*(rec + offset) |= 0x40;
			NTFS_PUTS32(rec + offset + coffs, rclus
							/* & 0xffffffffLL */);
			coffs += 4;
		} 
		offset += coffs;
		if (rl[i].lcn)
			cluster = rl[i].lcn;
	}
	if (offset >= size)
		return -E2BIG;
	/* Terminating null. */
	*(rec + offset++) = 0;
	*offs = offset;
	return 0;
}

static void count_runs(ntfs_attribute *attr, char *buf)
{
	ntfs_u32 first, count, last, i;
	
	first = 0;
	for (i = 0, count = 0; i < attr->d.r.len; i++)
		count += attr->d.r.runlist[i].len;
	last = first + count - 1;
	NTFS_PUTU64(buf + 0x10, first);
	NTFS_PUTU64(buf + 0x18, last);
} 

/**
 * layout_attr - convert in memory attribute to on disk attribute record
 * @attr:	in memory attribute to convert
 * @buf:	destination buffer for on disk attribute record
 * @size:	size of the destination buffer
 * @psize:	size of converted on disk attribute record (out variable)
 *
 * layout_attr() takes the attribute @attr and converts it into the appropriate
 * on disk structure, writing it into @buf (up to @size bytes are written).
 *
 * On success we return 0 and set @*psize to the actual byte size of the on-
 * disk attribute that was written into @buf.
 */
static int layout_attr(ntfs_attribute *attr, char *buf, int size, int *psize)
{
	int nameoff, hdrsize, asize;
	
	if (attr->resident) {
		nameoff = 0x18;
		hdrsize = (nameoff + 2 * attr->namelen + 7) & ~7;
		asize = (hdrsize + attr->size + 7) & ~7;
		if (size < asize)
			return -E2BIG;
		NTFS_PUTU32(buf + 0x10, attr->size);
		NTFS_PUTU8(buf + 0x16, attr->indexed);
		NTFS_PUTU16(buf + 0x14, hdrsize);
		if (attr->size)
			ntfs_memcpy(buf + hdrsize, attr->d.data, attr->size);
	} else {
		int error;

		if (attr->flags & ATTR_IS_COMPRESSED)
 			nameoff = 0x48;
 		else
 			nameoff = 0x40;
 		hdrsize = (nameoff + 2 * attr->namelen + 7) & ~7;
 		if (size < hdrsize)
 			return -E2BIG;
 		/* Make asize point at the end of the attribute record header,
		   i.e. at the beginning of the mapping pairs array. */
 		asize = hdrsize;
 		error = layout_runs(attr, buf, &asize, size);
 		/* Now, asize points one byte beyond the end of the mapping
		   pairs array. */
		if (error)
 			return error;
 		/* The next attribute has to begin on 8-byte boundary. */
		asize = (asize + 7) & ~7;
		count_runs(attr, buf);
		NTFS_PUTU16(buf + 0x20, hdrsize);
		NTFS_PUTU16(buf + 0x22, attr->cengine);
		NTFS_PUTU32(buf + 0x24, 0);
		NTFS_PUTS64(buf + 0x28, attr->allocated);
		NTFS_PUTS64(buf + 0x30, attr->size);
		NTFS_PUTS64(buf + 0x38, attr->initialized);
		if (attr->flags & ATTR_IS_COMPRESSED)
			NTFS_PUTS64(buf + 0x40, attr->compsize);
	}
	NTFS_PUTU32(buf, attr->type);
	NTFS_PUTU32(buf + 4, asize);
	NTFS_PUTU8(buf + 8, attr->resident ? 0 : 1);
	NTFS_PUTU8(buf + 9, attr->namelen);
	NTFS_PUTU16(buf + 0xa, nameoff);
	NTFS_PUTU16(buf + 0xc, attr->flags);
	NTFS_PUTU16(buf + 0xe, attr->attrno);
	if (attr->namelen)
		ntfs_memcpy(buf + nameoff, attr->name, 2 * attr->namelen);
	*psize = asize;
	return 0;
}

/**
 * layout_inode - convert an in-memory inode into on disk mft record(s)
 * @ino:	in memory inode to convert
 * @store:	on disk inode, contain buffers for the on disk mft record(s)
 *
 * layout_inode takes the in memory inode @ino, converts it into a (sequence of)
 * mft record(s) and writes them to the appropriate buffers in the @store.
 *
 * Return 0 on success,
 * the required mft record count (>0) if the inode does not fit,
 * -ENOMEM if memory allocation problem, or
 * -EOPNOTSUP if beyond our capabilities.
 *
 * TODO: We at the moment do not support extension mft records. (AIA)
 */
int layout_inode(ntfs_inode *ino, ntfs_disk_inode *store)
{
	int offset, i, size, psize, error, count, recno;
	ntfs_attribute *attr;
	unsigned char *rec;

	error = allocate_store(ino->vol, store, ino->record_count);
	if (error)
		return error;
	size = ino->vol->mft_record_size;
 	count = i = 0;
 	do {
 		if (count < ino->record_count) {
 			recno = ino->records[count];
 		} else {
 			error = allocate_store(ino->vol, store, count + 1);
 			if (error)
 				return error;
	 		recno = -1;
		}
	 	store->records[count].recno = recno;
 		rec = store->records[count].record;
	 	count++;
 		/* Copy mft record header. */
	 	offset = NTFS_GETU16(ino->attr + 0x14); /* attrs_offset */
		ntfs_memcpy(rec, ino->attr, offset);
	 	/* Copy attributes. */
 		while (i < ino->attr_count) {
 			attr = ino->attrs + i;
	 		error = layout_attr(attr, rec + offset,
					size - offset - 8, &psize);
	 		if (error == -E2BIG && offset != NTFS_GETU16(ino->attr
					+ 0x14))
 				break;
 			if (error)
 				return error;
 			offset += psize;
 			i++;
 		}
 		/* Terminating attribute. */
		NTFS_PUTU32(rec + offset, 0xFFFFFFFF);
		offset += 4;
		NTFS_PUTU32(rec + offset, 0);
		offset += 4;
		NTFS_PUTU32(rec + 0x18, offset);
	} while (i < ino->attr_count || count < ino->record_count);
	return count - ino->record_count;
}

int ntfs_update_inode(ntfs_inode *ino)
{
	int error, i;
	ntfs_disk_inode store;
	ntfs_io io;

	ntfs_bzero(&store, sizeof(store));
	error = layout_inode(ino, &store);
	if (error == -E2BIG) {
		i = ntfs_split_indexroot(ino);
		if (i != -ENOTDIR) {
			if (!i)
				i = layout_inode(ino, &store);
			error = i;
		}
	}
	if (error == -E2BIG) {
		error = ntfs_attr_allnonresident(ino);
		if (!error)
			error = layout_inode(ino, &store);
	}
	if (error > 0) {
		error = -E2BIG;
	}
	if (error) {
		if (error == -E2BIG)
			ntfs_error("Cannot handle saving inode 0x%x.\n",
				   ino->i_number);
		deallocate_store(&store);
		return error;
	}
	io.fn_get = ntfs_get;
	io.fn_put = 0;
	for (i = 0; i < store.count; i++) {
		error = ntfs_insert_fixups(store.records[i].record,
				ino->vol->mft_record_size);
		if (error) {
			printk(KERN_ALERT "NTFS: ntfs_update_inode() caught "
					"corrupt %s mtf record ntfs record "
					"header. Refusing to write corrupt "
					"data to disk. Unmount and run chkdsk "
					"immediately!\n", i ? "extension":
					"base");
			deallocate_store(&store);
			return -EIO;
		}
		io.param = store.records[i].record;
		io.size = ino->vol->mft_record_size;
		error = ntfs_write_attr(ino->vol->mft_ino, ino->vol->at_data,
				0, (__s64)store.records[i].recno <<
				ino->vol->mft_record_size_bits, &io);
		if (error || io.size != ino->vol->mft_record_size) {
			/* Big trouble, partially written file. */
			ntfs_error("Please unmount: Write error in inode "
					"0x%x\n", ino->i_number);
			deallocate_store(&store);
			return error ? error : -EIO;
		}
	}
	deallocate_store(&store);
	return 0;
}	

void ntfs_decompress(unsigned char *dest, unsigned char *src, ntfs_size_t l)
{
	int head, comp;
	int copied = 0;
	unsigned char *stop;
	int bits;
	int tag = 0;
	int clear_pos;
	
	while (1) {
		head = NTFS_GETU16(src) & 0xFFF;
		/* High bit indicates that compression was performed. */
		comp = NTFS_GETU16(src) & 0x8000;
		src += 2;
		stop = src + head;
		bits = 0;
		clear_pos = 0;
		if (head == 0)
			/* Block is not used. */
			return;
		if (!comp) { /* uncompressible */
			ntfs_memcpy(dest, src, 0x1000);
			dest += 0x1000;
			copied += 0x1000;
			src += 0x1000;
			if (l == copied)
				return;
			continue;
		}
		while (src <= stop) {
			if (clear_pos > 4096) {
				ntfs_error("Error 1 in decompress\n");
				return;
			}
			if (!bits) {
				tag = NTFS_GETU8(src);
				bits = 8;
				src++;
				if (src > stop)
					break;
			}
			if (tag & 1) {
				int i, len, delta, code, lmask, dshift;
				code = NTFS_GETU16(src);
				src += 2;
				if (!clear_pos) {
					ntfs_error("Error 2 in decompress\n");
					return;
				}
				for (i = clear_pos - 1, lmask = 0xFFF,
				     dshift = 12; i >= 0x10; i >>= 1) {
					lmask >>= 1;
					dshift--;
				}
				delta = code >> dshift;
				len = (code & lmask) + 3;
				for (i = 0; i < len; i++) {
					dest[clear_pos] = dest[clear_pos - 
								    delta - 1];
					clear_pos++;
					copied++;
					if (copied==l)
						return;
				}
			} else {
				dest[clear_pos++] = NTFS_GETU8(src);
				src++;
				copied++;
				if (copied==l)
					return;
			}
			tag >>= 1;
			bits--;
		}
		dest += clear_pos;
	}
}

/*
 * NOTE: Neither of the ntfs_*_bit functions are atomic! But we don't need
 * them atomic at present as we never operate on shared/cached bitmaps.
 */
static __inline__ int ntfs_test_bit(unsigned char *byte, const int bit)
{
	return byte[bit >> 3] & (1 << (bit & 7)) ? 1 : 0;
}

static __inline__ void ntfs_set_bit(unsigned char *byte, const int bit)
{
	byte[bit >> 3] |= 1 << (bit & 7);
}

static __inline__ void ntfs_clear_bit(unsigned char *byte, const int bit)
{
	byte[bit >> 3] &= ~(1 << (bit & 7));
}

static __inline__ int ntfs_test_and_clear_bit(unsigned char *byte,
		const int bit)
{
	unsigned char *ptr = byte + (bit >> 3);
	int b = 1 << (bit & 7);
	int oldbit = *ptr & b ? 1 : 0;
	*ptr &= ~b;
	return oldbit;
}

static void dump_runlist(const ntfs_runlist *rl, const int rlen)
{
#ifdef DEBUG
	int i;
	ntfs_cluster_t ct;

	ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): rlen = %i.\n", rlen);
	ntfs_debug(DEBUG_OTHER, "VCN        LCN        Run length\n");
	for (i = 0, ct = 0; i < rlen; ct += rl[i++].len) {
		if (rl[i].lcn == (ntfs_cluster_t)-1)
			ntfs_debug(DEBUG_OTHER, "0x%-8x LCN_HOLE   0x%-8x "
					"(%s)\n", ct, rl[i].len, rl[i].len ?
					"sparse run" : "run list end");
		else
			ntfs_debug(DEBUG_OTHER, "0x%-8x 0x%-8x 0x%-8x%s\n", ct,
					rl[i].lcn, rl[i].len, rl[i].len &&
					i + 1 < rlen ? "" : " (run list end)");
		if (!rl[i].len)
			break;
	}
#endif
}

/**
 * splice_runlists - splice two run lists into one
 * @rl1:	pointer to address of first run list
 * @r1len:	number of elementfs in first run list
 * @rl2:	pointer to second run list
 * @r2len:	number of elements in second run list
 *
 * Append the run list @rl2 to the run list *@rl1 and return the result in
 * *@rl1 and *@r1len.
 *
 * Return 0 on success or -errno on error, in which case *@rl1 and *@r1len are
 * left untouched.
 *
 * The only possible error code at the moment is -ENOMEM and only happens if
 * there is insufficient memory to allocate the new run list (only happens
 * when size of (rl1 + rl2) > allocated size of rl1).
 */
int splice_runlists(ntfs_runlist **rl1, int *r1len, const ntfs_runlist *rl2,
		int r2len)
{
	ntfs_runlist *rl;
	int rlen, rl_size, rl2_pos;

	ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): Entering with *r1len = %i, "
			"r2len = %i.\n", *r1len, r2len);
	ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): Dumping 1st runlist.\n");
	if (*rl1)
		dump_runlist(*rl1, *r1len);
	else
		ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): Not present.\n");
	ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): Dumping 2nd runlist.\n");
	dump_runlist(rl2, r2len);
	rlen = *r1len + r2len + 1;
	rl_size = (rlen * sizeof(ntfs_runlist) + PAGE_SIZE - 1) &
			PAGE_MASK;
	ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): rlen = %i, rl_size = %i.\n",
			rlen, rl_size);
	/* Do we have enough space? */
	if (rl_size <= ((*r1len * sizeof(ntfs_runlist) + PAGE_SIZE - 1) &
			PAGE_MASK)) {
		/* Have enough space already. */
		rl = *rl1;
		ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): Have enough space "
				"already.\n");
	} else {
		/* Need more space. Reallocate. */
		ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): Need more space.\n");
		rl = ntfs_vmalloc(rlen << sizeof(ntfs_runlist));
		if (!rl)
			return -ENOMEM;
		/* Copy over rl1. */
		ntfs_memcpy(rl, *rl1, *r1len * sizeof(ntfs_runlist));
		ntfs_vfree(*rl1);
		*rl1 = rl;
	}
	/* Reuse rl_size as the current position index into rl. */
	rl_size = *r1len - 1;
	ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): rl_size = %i.\n");
	/* Coalesce neighbouring elements, if present. */
	rl2_pos = 0;
	if (rl[rl_size].lcn + rl[rl_size].len == rl2[rl2_pos].lcn) {
		ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): Coalescing adjacent "
				"runs.\n");
		ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): Before: "
				"rl[rl_size].len = %i.\n", rl[rl_size].len);
		rl[rl_size].len += rl2[rl2_pos].len;
		ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): After: "
				"rl[rl_size].len = %i.\n", rl[rl_size].len);
		rl2_pos++;
		r2len--;
		rlen--;
	}
	rl_size++;
	/* Copy over rl2. */
	ntfs_memcpy(rl + rl_size, rl2 + rl2_pos, r2len * sizeof(ntfs_runlist));
	rlen--;
	rl[rlen].lcn = (ntfs_cluster_t)-1;
	rl[rlen].len = (ntfs_cluster_t)0;
	*r1len = rlen;
	ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): Dumping result runlist.\n");
	dump_runlist(*rl1, *r1len);
	ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): Returning with *r1len = "
			"%i.\n", rlen);
	return 0;
}

static int ntfs_alloc_mft_record(ntfs_volume *vol, unsigned long *result)
{
	unsigned long nr_mft_records, buf_size, buf_pos, pass_start, pass_end;
	unsigned long last_read_pos, mft_rec_size, bit, l;
	ntfs_attribute *data, *bmp;
	__u8 *buf, *byte, pass, b, have_allocated_mftbmp = 0;
	int rlen, rl_size = 0, r2len, rl2_size, old_data_rlen, err = 0;
	ntfs_runlist *rl, *rl2;
	ntfs_cluster_t lcn = 0, old_data_len;
	ntfs_io io;
	__s64 ll, old_data_allocated, old_data_initialized, old_data_size;

	*result = -1UL;
	/* Allocate a buffer and setup the io structure. */
	buf = (__u8*)__get_free_page(GFP_NOFS);
	if (!buf)
		return -ENOMEM;
	lock_kernel();
	/* Get the $DATA and $BITMAP attributes of $MFT. */
	data = ntfs_find_attr(vol->mft_ino, vol->at_data, 0);
	bmp = ntfs_find_attr(vol->mft_ino, vol->at_bitmap, 0);
	if (!data || !bmp) {
		err = -EINVAL;
		goto err_ret;
	}
	/* Determine the number of allocated mft records in the mft. */
	pass_end = nr_mft_records = data->allocated >>
			vol->mft_record_size_bits;
	ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): nr_mft_records = %lu.\n",
			nr_mft_records);
	/* Make sure we don't overflow the bitmap. */
	l = bmp->initialized << 3;
	if (l < nr_mft_records)
		// FIXME: It might be a good idea to extend the bitmap instead.
		pass_end = l;
	pass = 1;
	buf_pos = vol->mft_data_pos;
	if (buf_pos >= pass_end) {
		buf_pos = 24UL;
		pass = 2;
	}
	pass_start = buf_pos;
	rl = bmp->d.r.runlist;
	rlen = bmp->d.r.len - 1;
	lcn = rl[rlen].lcn + rl[rlen].len;
	io.fn_put = ntfs_put;
	io.fn_get = ntfs_get;
	ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): Starting bitmap search.\n");
	ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): pass = %i, pass_start = %lu, "
			"pass_end = %lu.\n", pass, pass_start, pass_end);
	byte = NULL; // FIXME: For debugging only.
	/* Loop until a free mft record is found. */
	io.size = (nr_mft_records >> 3) & ~PAGE_MASK;
	for (;; io.size = PAGE_SIZE) {
		io.param = buf;
		io.do_read = 1;
		last_read_pos = buf_pos >> 3;
		ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): Before: "
				"bmp->allocated = 0x%Lx, bmp->size = 0x%Lx, "
				"bmp->initialized = 0x%Lx.\n", bmp->allocated,
				bmp->size, bmp->initialized);
		err = ntfs_readwrite_attr(vol->mft_ino, bmp, last_read_pos,
				&io);
		if (err)
			goto err_ret;
		ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): Read %lu bytes.\n",
				(unsigned long)io.size);
		ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): After: "
				"bmp->allocated = 0x%Lx, bmp->size = 0x%Lx, "
				"bmp->initialized = 0x%Lx.\n", bmp->allocated,
				bmp->size, bmp->initialized);
		if (!io.size)
			goto pass_done;
		buf_size = io.size << 3;
		bit = buf_pos & 7UL;
		buf_pos &= ~7UL;
		ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): Before loop: "
				"buf_size = %lu, buf_pos = %lu, bit = %lu, "
				"*byte = 0x%x, b = %u.\n",
				buf_size, buf_pos, bit, byte ? *byte : -1, b);
		for (; bit < buf_size && bit + buf_pos < pass_end;
				bit &= ~7UL, bit += 8UL) {
			byte = buf + (bit >> 3);
			if (*byte == 0xff)
				continue;
			b = ffz((unsigned long)*byte);
			if (b < (__u8)8 && b >= (bit & 7UL)) {
				bit = b + (bit & ~7UL) + buf_pos;
				ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): "
						"Found free rec in for loop. "
						"bit = %lu\n", bit);
				goto found_free_rec;
			}
		}
		ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): After loop: "
				"buf_size = %lu, buf_pos = %lu, bit = %lu, "
				"*byte = 0x%x, b = %u.\n",
				buf_size, buf_pos, bit, byte ? *byte : -1, b);
		buf_pos += buf_size;
		if (buf_pos < pass_end)
			continue;
pass_done:	/* Finished with the current pass. */
		ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): At pass_done.\n");
		if (pass == 1) {
			/*
			 * Now do pass 2, scanning the first part of the zone
			 * we omitted in pass 1.
			 */
			ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): Done pass "
					"1.\n");
			ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): Pass = 2.\n");
			pass = 2;
			pass_end = pass_start;
			buf_pos = pass_start = 24UL;
			ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): pass = %i, "
					"pass_start = %lu, pass_end = %lu.\n",
					pass, pass_start, pass_end);
			continue;
		} /* pass == 2 */
		/* No free records left. */
		if (bmp->initialized << 3 > nr_mft_records &&
				bmp->initialized > 3) {
			/*
			 * The mft bitmap is already bigger but the space is
			 * not covered by mft records, this implies that the
			 * next records are all free, so we already have found
			 * a free record.
			 */
			bit = nr_mft_records;
			if (bit < 24UL)
				bit = 24UL;
			ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): Found free "
					"record bit (#1) = 0x%lx.\n", bit);
			goto found_free_rec;
		}
		ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): Done pass 2.\n");
		ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): Before: "
				"bmp->allocated = 0x%Lx, bmp->size = 0x%Lx, "
				"bmp->initialized = 0x%Lx.\n", bmp->allocated,
				bmp->size, bmp->initialized);
		/* Need to extend the mft bitmap. */
		if (bmp->initialized + 8LL > bmp->allocated) {
			ntfs_io io2;

			ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): Initialized "
					"> allocated.\n");
			/* Need to extend bitmap by one more cluster. */
			rl = bmp->d.r.runlist;
			rlen = bmp->d.r.len - 1;
			lcn = rl[rlen].lcn + rl[rlen].len;
			io2.fn_put = ntfs_put;
			io2.fn_get = ntfs_get;
			io2.param = &b;
			io2.size = 1;
			io2.do_read = 1;
			err = ntfs_readwrite_attr(vol->bitmap, data, lcn >> 3,
					&io2);
			if (err)
				goto err_ret;
			ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): Read %lu "
					"bytes.\n", (unsigned long)io2.size);
			if (io2.size == 1 && b != 0xff) {
				__u8 tb = 1 << (lcn & (ntfs_cluster_t)7);
				if (!(b & tb)) {
					/* Next cluster is free. Allocate it. */
					b |= tb;
					io2.param = &b;
					io2.do_read = 0;
					err = ntfs_readwrite_attr(vol->bitmap,
							data, lcn >> 3, &io2);
					if (err || io.size != 1) {
						if (!err)
							err = -EIO;
						goto err_ret;
					}
append_mftbmp_simple:			rl[rlen].len++;
					have_allocated_mftbmp |= 1;
					ntfs_debug(DEBUG_OTHER, __FUNCTION__
							"(): Appending one "
							"cluster to mftbmp.\n");
				}
			}
			if (!have_allocated_mftbmp) {
				/* Allocate a cluster from the DATA_ZONE. */
				ntfs_cluster_t lcn2 = lcn;
				ntfs_cluster_t count = 1;
				err = ntfs_allocate_clusters(vol, &lcn2,
						&count, &rl2, &r2len,
						DATA_ZONE);
				if (err)
					goto err_ret;
				if (count != 1 || lcn2 <= 0) {
					if (count > 0) {
rl2_dealloc_err_out:				if (ntfs_deallocate_clusters(
							vol, rl2, r2len))
							ntfs_error(__FUNCTION__
							"(): Cluster "
							"deallocation in error "
							"code path failed! You "
							"should run chkdsk.\n");
					}
					ntfs_vfree(rl2);
					if (!err)
						err = -EINVAL;
					goto err_ret;
				}
				if (lcn2 == lcn) {
					ntfs_vfree(rl2);
					goto append_mftbmp_simple;
				}
				/* We need to append a new run. */
				rl_size = (rlen * sizeof(ntfs_runlist) +
						PAGE_SIZE - 1) & PAGE_MASK;
				/* Reallocate memory if necessary. */
				if ((rlen + 2) * sizeof(ntfs_runlist) >=
						rl_size) {
					ntfs_runlist *rlt;

					rl_size += PAGE_SIZE;
					rlt = ntfs_vmalloc(rl_size);
					if (!rlt) {
						err = -ENOMEM;
						goto rl2_dealloc_err_out;
					}
					ntfs_memcpy(rlt, rl, rl_size -
							PAGE_SIZE);
					ntfs_vfree(rl);
					bmp->d.r.runlist = rl = rlt;
				}
				ntfs_vfree(rl2);
				rl[rlen].lcn = lcn = lcn2;
				rl[rlen].len = count;
				bmp->d.r.len = ++rlen;
				have_allocated_mftbmp |= 2;
				ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): "
						"Adding run to mftbmp. "
						"LCN = %i, len = %i\n", lcn,
						count);
			}
			/*
			 * We now have extended the mft bitmap allocated size
			 * by one cluster. Reflect this in the attribute.
			 */
			bmp->allocated += (__s64)vol->cluster_size;
		}
		ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): After: "
				"bmp->allocated = 0x%Lx, bmp->size = 0x%Lx, "
				"bmp->initialized = 0x%Lx.\n", bmp->allocated,
				bmp->size, bmp->initialized);
		/* We now have sufficient allocated space. */
		ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): Now have sufficient "
				"allocated space in mftbmp.\n");
		ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): Before: "
				"bmp->allocated = 0x%Lx, bmp->size = 0x%Lx, "
				"bmp->initialized = 0x%Lx.\n", bmp->allocated,
				bmp->size, bmp->initialized);
		buf_pos = bmp->initialized;
		bmp->initialized += 8LL;
		if (bmp->initialized > bmp->size)
			bmp->size = bmp->initialized;
		ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): After: "
				"bmp->allocated = 0x%Lx, bmp->size = 0x%Lx, "
				"bmp->initialized = 0x%Lx.\n", bmp->allocated,
				bmp->size, bmp->initialized);
		have_allocated_mftbmp |= 4;
		/* Update the mft bitmap attribute value. */
		memset(buf, 0, 8);
		io.param = buf;
		io.size = 8;
		io.do_read = 0;
		err = ntfs_readwrite_attr(vol->mft_ino, bmp, buf_pos, &io);
		if (err || io.size != 8) {
			if (!err)
				err = -EIO;
			goto shrink_mftbmp_err_ret;
		}
		ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): Wrote extended "
				"mftbmp bytes %lu.\n", (unsigned long)io.size);
		ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): After write: "
				"bmp->allocated = 0x%Lx, bmp->size = 0x%Lx, "
				"bmp->initialized = 0x%Lx.\n", bmp->allocated,
				bmp->size, bmp->initialized);
		bit = buf_pos << 3;
		ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): Found free record "
				"bit (#2) = 0x%lx.\n", bit);
		goto found_free_rec;
	}
found_free_rec:
	/* bit is the found free mft record. Allocate it in the mft bitmap. */
	vol->mft_data_pos = bit;
	ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): At found_free_rec.\n");
	io.param = buf;
	io.size = 1;
	io.do_read = 1;
	ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): Before update: "
			"bmp->allocated = 0x%Lx, bmp->size = 0x%Lx, "
			"bmp->initialized = 0x%Lx.\n", bmp->allocated,
			bmp->size, bmp->initialized);
	err = ntfs_readwrite_attr(vol->mft_ino, bmp, bit >> 3, &io);
	if (err || io.size != 1) {
		if (!err)
			err = -EIO;
		goto shrink_mftbmp_err_ret;
	}
	ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): Read %lu bytes.\n",
			(unsigned long)io.size);
#ifdef DEBUG
	/* Check our bit is really zero! */
	if (*buf & (1 << (bit & 7)))
		BUG();
#endif
	*buf |= 1 << (bit & 7);
	io.param = buf;
	io.do_read = 0;
	err = ntfs_readwrite_attr(vol->mft_ino, bmp, bit >> 3, &io);
	if (err || io.size != 1) {
		if (!err)
			err = -EIO;
		goto shrink_mftbmp_err_ret;
	}
	ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): Wrote %lu bytes.\n",
			(unsigned long)io.size);
	ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): After update: "
			"bmp->allocated = 0x%Lx, bmp->size = 0x%Lx, "
			"bmp->initialized = 0x%Lx.\n", bmp->allocated,
			bmp->size, bmp->initialized);
	/* The mft bitmap is now uptodate. Deal with mft data attribute now. */
	ll = (__s64)(bit + 1) << vol->mft_record_size_bits;
	if (ll <= data->initialized) {
		/* The allocated record is already initialized. We are done! */
		ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): Allocated mft record "
				"already initialized!\n");
		goto done_ret;
	}
	ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): Allocated mft record needs "
			"to be initialized.\n");
	/* The mft record is outside the initialized data. */
	mft_rec_size = (unsigned long)vol->mft_record_size;
	/* Preserve old values for undo purposes. */
	old_data_allocated = data->allocated;
	old_data_rlen = data->d.r.len - 1;
	old_data_len = data->d.r.runlist[old_data_rlen].len;
	/*
	 * If necessary, extend the mft until it covers the allocated record.
	 * The loop is only actually used when a freshly formatted volume is
	 * first written to. But it optimizes away nicely in the common case.
	 */
	while (ll > data->allocated) {
		ntfs_cluster_t lcn2, nr_lcn2, nr, min_nr;

		ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): Extending mft "
				"data allocation, data->allocated = 0x%Lx, "
				"data->size = 0x%Lx, data->initialized = "
				"0x%Lx.\n", data->allocated, data->size,
				data->initialized);
		/* Minimum allocation is one mft record worth of clusters. */
		if (mft_rec_size <= vol->cluster_size)
			min_nr = (ntfs_cluster_t)1;
		else
			min_nr = mft_rec_size >> vol->cluster_size_bits;
		ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): min_nr = %i.\n",
				min_nr);
		/* Allocate 16 mft records worth of clusters. */
		nr = mft_rec_size << 4 >> vol->cluster_size_bits;
		if (!nr)
			nr = (ntfs_cluster_t)1;
		/* Determine the preferred allocation location. */
		ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): nr = %i.\n", nr);
		rl2 = data->d.r.runlist;
		r2len = data->d.r.len;
		lcn2 = rl2[r2len - 1].lcn + rl2[r2len - 1].len;
		ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): rl2[r2len - 1].lcn "
				"= %i, .len = %i.\n", rl2[r2len - 1].lcn,
				rl2[r2len - 1].len);
		ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): lcn2 = %i, r2len = "
				"%i.\n", lcn2, r2len);
retry_mft_data_allocation:
		nr_lcn2 = nr;
		err = ntfs_allocate_clusters(vol, &lcn2, &nr_lcn2, &rl2,
				&r2len, MFT_ZONE);
#ifdef DEBUG
		if (!err && nr_lcn2 < min_nr)
			/* Allocated less than minimum needed. Weird! */
			BUG();
#endif
		if (err) {
			/*
			 * If there isn't enough space to do the wanted
			 * allocation, but there is enough space to do a
			 * minimal allocation, then try that, unless the wanted
			 * allocation was already the minimal allocation.
			 */
			if (err == -ENOSPC && nr > min_nr &&
					nr_lcn2 >= min_nr) {
				nr = min_nr;
				ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): "
						"Retrying mft data "
						"allocation, nr = min_nr = %i"
						".\n", nr);
				goto retry_mft_data_allocation;
			}
			goto undo_mftbmp_alloc_err_ret;
		}
		ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): Allocated %i "
				"clusters starting at LCN %i.\n", nr_lcn2,
				lcn2);
		ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): Allocated "
				"runlist:\n");
		dump_runlist(rl2, r2len);
		/* Append rl2 to the mft data attribute's run list. */
		err = splice_runlists(&data->d.r.runlist, (int*)&data->d.r.len,
				rl2, r2len);
		if (err) {
			ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): "
					"splice_runlists failed with error "
					"code %i.\n", -err);
			goto undo_partial_data_alloc_err_ret;
		}
		/* Reflect the allocated clusters in the mft allocated data. */
		data->allocated += nr_lcn2 << vol->cluster_size_bits;
		ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): After extending mft "
				"data allocation, data->allocated = 0x%Lx, "
				"data->size = 0x%Lx, data->initialized = "
				"0x%Lx.\n", data->allocated, data->size,
				data->initialized);
	}
	/* Prepare a formatted (empty) mft record. */
	memset(buf, 0, mft_rec_size);
	ntfs_fill_mft_header(buf, mft_rec_size, 0, 0, 0);
	err = ntfs_insert_fixups(buf, mft_rec_size);
	if (err)
		goto undo_data_alloc_err_ret;
	/*
	 * Extend mft data initialized size to reach the allocated mft record
	 * and write the formatted mft record buffer to each mft record being
	 * initialized. Note, that ntfs_readwrite_attr extends both
	 * data->initialized and data->size, so no need for us to touch them.
	 */
	old_data_initialized = data->initialized;
	old_data_size = data->size;
	while (ll > data->initialized) {
		ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): Initializing mft "
				"record 0x%Lx.\n",
				data->initialized >> vol->mft_record_size_bits);
		io.param = buf;
		io.size = mft_rec_size;
		io.do_read = 0;
		err = ntfs_readwrite_attr(vol->mft_ino, data,
				data->initialized, &io);
		if (err || io.size != mft_rec_size) {
			if (!err)
				err = -EIO;
			goto undo_data_init_err_ret;
		}
		ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): Wrote %i bytes to "
				"mft data.\n", io.size);
	}
	/* Update the VFS inode size as well. */
	VFS_I(vol->mft_ino)->i_size = data->size;
#ifdef DEBUG
	ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): After mft record "
			"initialization: data->allocated = 0x%Lx, data->size "
			"= 0x%Lx, data->initialized = 0x%Lx.\n",
			data->allocated, data->size, data->initialized);
	/* Sanity checks. */
	if (data->size > data->allocated || data->size < data->initialized ||
			data->initialized > data->allocated)
		BUG();
#endif
done_ret:
	/* Return the number of the allocated mft record. */
	ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): At done_ret. *result = bit = "
			"0x%lx.\n", bit);
	*result = bit;
	vol->mft_data_pos = bit + 1;
err_ret:
	unlock_kernel();
	free_page((unsigned long)buf);
	ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): Syncing inode $MFT.\n");
	if (ntfs_update_inode(vol->mft_ino))
		ntfs_error(__FUNCTION__ "(): Failed to sync inode $MFT. "
				"Continuing anyway.\n");
	if (!err) {
		ntfs_debug(DEBUG_FILE3, __FUNCTION__ "(): Done. Allocated mft "
				"record number *result = 0x%lx.\n", *result);
		return 0;
	}
	if (err != -ENOSPC)
		ntfs_error(__FUNCTION__ "(): Failed to allocate an mft "
				"record. Returning error code %i.\n", -err);
	else
		ntfs_debug(DEBUG_FILE3, __FUNCTION__ "(): Failed to allocate "
				"an mft record due to lack of free space.\n");
	return err;
undo_data_init_err_ret:
	ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): At "
			"undo_data_init_err_ret.\n");
	data->initialized = old_data_initialized;
	data->size = old_data_size;
undo_data_alloc_err_ret:
	ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): At undo_data_alloc_err_ret."
			"\n");
	data->allocated = old_data_allocated;
undo_partial_data_alloc_err_ret:
	ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): At "
			"undo_partial_data_alloc_err_ret.\n");
	/* Deallocate the clusters. */
	if (ntfs_deallocate_clusters(vol, rl2, r2len))
		ntfs_error(__FUNCTION__ "(): Error deallocating clusters in "
				"error code path. You should run chkdsk.\n");
	ntfs_vfree(rl2);
	/* Revert the run list back to what it was before. */
	r2len = data->d.r.len;
	rl2 = data->d.r.runlist;
	rl2[old_data_rlen++].len = old_data_len;
	rl2[old_data_rlen].lcn = (ntfs_cluster_t)-1;
	rl2[old_data_rlen].len = (ntfs_cluster_t)0;
	data->d.r.len = old_data_rlen;
	rl2_size = ((old_data_rlen + 1) * sizeof(ntfs_runlist) + PAGE_SIZE -
			1) & PAGE_MASK;
	/* Reallocate memory freeing any extra memory allocated. */
	if (rl2_size < ((r2len * sizeof(ntfs_runlist) + PAGE_SIZE - 1) &
			PAGE_MASK)) {
		rl2 = ntfs_vmalloc(rl2_size);
		if (rl2) {
			ntfs_memcpy(rl2, data->d.r.runlist, rl2_size);
			ntfs_vfree(data->d.r.runlist);
			data->d.r.runlist = rl2;
		} else
			ntfs_error(__FUNCTION__ "(): Error reallocating "
					"memory in error code path. This "
					"should be harmless.\n");
	}	
undo_mftbmp_alloc_err_ret:
	ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): At "
			"undo_mftbmp_alloc_err_ret.\n");
	/* Deallocate the allocated bit in the mft bitmap. */
	io.param = buf;
	io.size = 1;
	io.do_read = 1;
	err = ntfs_readwrite_attr(vol->mft_ino, bmp, bit >> 3, &io);
	if (!err && io.size == 1) {
		*buf &= ~(1 << (bit & 7));
		io.param = buf;
		io.do_read = 0;
		err = ntfs_readwrite_attr(vol->mft_ino, bmp, bit >> 3, &io);
	}
	if (err || io.size != 1) {
		if (!err)
			err = -EIO;
		ntfs_error(__FUNCTION__ "(): Error deallocating mft record in "
				"error code path. You should run chkdsk.\n");
	}
shrink_mftbmp_err_ret:
	ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): At shrink_mftbmp_err_ret.\n");
	ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): have_allocated_mftbmp = "
			"%i.\n", have_allocated_mftbmp);
	if (!have_allocated_mftbmp)
		goto err_ret;
	/* Shrink the mftbmp back to previous size. */
	if (bmp->size == bmp->initialized)
		bmp->size -= 8LL;
	bmp->initialized -= 8LL;
	have_allocated_mftbmp &= ~4;
	/* If no allocation occured then we are done. */
	ntfs_debug(DEBUG_OTHER, __FUNCTION__ "(): have_allocated_mftbmp = "
			"%i.\n", have_allocated_mftbmp);
	if (!have_allocated_mftbmp)
		goto err_ret;
	/* Deallocate the allocated cluster. */
	bmp->allocated -= (__s64)vol->cluster_size;
	if (ntfs_deallocate_cluster_run(vol, lcn, (ntfs_cluster_t)1))
		ntfs_error(__FUNCTION__ "(): Error deallocating cluster in "
				"error code path. You should run chkdsk.\n");
	switch (have_allocated_mftbmp & 3) {
	case 1:
		/* Delete the last lcn from the last run of mftbmp. */
		rl[rlen - 1].len--;
		break;
	case 2:
		/* Delete the last run of mftbmp. */
		bmp->d.r.len = --rlen;
		/* Reallocate memory if necessary. */
		if ((rlen + 1) * sizeof(ntfs_runlist) <= rl_size - PAGE_SIZE) {
			ntfs_runlist *rlt;

			rl_size -= PAGE_SIZE;
			rlt = ntfs_vmalloc(rl_size);
			if (rlt) {
				ntfs_memcpy(rlt, rl, rl_size);
				ntfs_vfree(rl);
				bmp->d.r.runlist = rl = rlt;
			} else
				ntfs_error(__FUNCTION__ "(): Error "
						"reallocating memory in error "
						"code path. This should be "
						"harmless.\n");
		}
		bmp->d.r.runlist[bmp->d.r.len].lcn = (ntfs_cluster_t)-1;
		bmp->d.r.runlist[bmp->d.r.len].len = (ntfs_cluster_t)0;
		break;
	default:
		BUG();
	}
	goto err_ret;
}

/* We need 0x48 bytes in total. */
static int add_standard_information(ntfs_inode *ino)
{
	ntfs_time64_t now;
	char data[0x30];
	char *position = data;
	ntfs_attribute *si;

	now = ntfs_now();
	NTFS_PUTU64(position + 0x00, now);		/* File creation */
	NTFS_PUTU64(position + 0x08, now);		/* Last modification */
	NTFS_PUTU64(position + 0x10, now);		/* Last mod for MFT */
	NTFS_PUTU64(position + 0x18, now);		/* Last access */
	NTFS_PUTU64(position + 0x20, 0);		/* MSDOS file perms */
	NTFS_PUTU64(position + 0x28, 0);		/* unknown */
	return ntfs_create_attr(ino, ino->vol->at_standard_information, 0,
			data, sizeof(data), &si);
}

static int add_filename(ntfs_inode *ino, ntfs_inode *dir, 
		const unsigned char *filename, int length, ntfs_u32 flags)
{
	unsigned char *position;
	unsigned int size;
	ntfs_time64_t now;
	int count, error;
	unsigned char* data;
	ntfs_attribute *fn;

	/* Work out the size. */
	size = 0x42 + 2 * length;
	data = ntfs_malloc(size);
	if (!data)
		return -ENOMEM;
	/* Search for a position. */
	position = data;
	NTFS_PUTINUM(position, dir);			/* Inode num of dir */
	now = ntfs_now();
	NTFS_PUTU64(position + 0x08, now);		/* File creation */
	NTFS_PUTU64(position + 0x10, now);		/* Last modification */
	NTFS_PUTU64(position + 0x18, now);		/* Last mod for MFT */
	NTFS_PUTU64(position + 0x20, now);		/* Last access */
	NTFS_PUTS64(position + 0x28, 0);		/* Allocated size */
	NTFS_PUTS64(position + 0x30, 0);		/* Data size */
	NTFS_PUTU32(position + 0x38, flags);		/* File flags */
	NTFS_PUTU32(position + 0x3c, 0);		/* We don't use these
							 * features yet. */
	NTFS_PUTU8(position + 0x40, length);		/* Filename length */
	NTFS_PUTU8(position + 0x41, 0);			/* Only long name */
	position += 0x42;
	for (count = 0; count < length; count++) {
		NTFS_PUTU16(position + 2 * count, filename[count]);
	}
	error = ntfs_create_attr(ino, ino->vol->at_file_name, 0, data, size,
				 &fn);
	if (!error)
		error = ntfs_dir_add(dir, ino, fn);
	ntfs_free(data);
	return error;
}

int add_security(ntfs_inode* ino, ntfs_inode* dir)
{
	int error;
	char *buf;
	int size;
	ntfs_attribute* attr;
	ntfs_io io;
	ntfs_attribute *se;

	attr = ntfs_find_attr(dir, ino->vol->at_security_descriptor, 0);
	if (!attr)
		return -EOPNOTSUPP; /* Need security in directory. */
	size = attr->size;
	if (size > 512)
		return -EOPNOTSUPP;
	buf = ntfs_malloc(size);
	if (!buf)
		return -ENOMEM;
	io.fn_get = ntfs_get;
	io.fn_put = ntfs_put;
	io.param = buf;
	io.size = size;
	error = ntfs_read_attr(dir, ino->vol->at_security_descriptor, 0, 0,&io);
	if (!error && io.size != size)
		ntfs_error("wrong size in add_security\n");
	if (error) {
		ntfs_free(buf);
		return error;
	}
	error = ntfs_create_attr(ino, ino->vol->at_security_descriptor,
				 0, buf, size, &se);
	ntfs_free(buf);
	return error;
}

static int add_data(ntfs_inode* ino, unsigned char *data, int length)
{
	ntfs_attribute *da;
	
	return ntfs_create_attr(ino, ino->vol->at_data, 0, data, length, &da);
}

int ntfs_alloc_inode(ntfs_inode *dir, ntfs_inode *result, const char *filename,
		int namelen, ntfs_u32 flags)
{
	ntfs_volume *vol = dir->vol;
	int err;
	ntfs_u8 buffer[2];
	ntfs_io io;

	err = ntfs_alloc_mft_record(vol, &(result->i_number));
	if (err) {
		if (err == -ENOSPC)
			ntfs_error(__FUNCTION__ "(): No free inodes.\n");
		return err;
	}
	/* Get the sequence number. */
	io.fn_put = ntfs_put;
	io.fn_get = ntfs_get;
	io.param = buffer;
	io.size = 2;
	err = ntfs_read_attr(vol->mft_ino, vol->at_data, 0, 
			((__s64)result->i_number << vol->mft_record_size_bits)
			+ 0x10, &io);
	// FIXME: We are leaving the MFT in inconsistent state! (AIA)
	if (err)
		return err;
	/* Increment the sequence number skipping zero. */
	result->sequence_number = (NTFS_GETU16(buffer) + 1) & 0xffff;
	if (!result->sequence_number)
		result->sequence_number++;
	result->vol = vol;
	result->attr_count = 0;
	result->attrs = 0;
	result->record_count = 1;
	result->records = ntfs_calloc(8 * sizeof(int));
	if (!result->records)
		goto mem_err_out;
	result->records[0] = result->i_number;
	result->attr = ntfs_calloc(vol->mft_record_size);
	if (!result->attr) {
		ntfs_free(result->records);
		result->records = NULL;
		goto mem_err_out;
	}
	ntfs_fill_mft_header(result->attr, vol->mft_record_size,
			result->sequence_number, 1, 1);
	err = add_standard_information(result);
	if (!err)
		err = add_filename(result, dir, filename, namelen, flags);
	if (!err)
		err = add_security(result, dir);
	// FIXME: We are leaving the MFT in inconsistent state on error! (AIA)
	return err;
mem_err_out:
	// FIXME: We are leaving the MFT in inconsistent state! (AIA)
	result->record_count = 0;
	result->attr = NULL;
	return -ENOMEM;
}

int ntfs_alloc_file(ntfs_inode *dir, ntfs_inode *result, char *filename,
		int namelen)
{
	int err;

	err = ntfs_alloc_inode(dir, result, filename, namelen, 0);
	if (!err)
		err = add_data(result, 0, 0);
	return err;
}

