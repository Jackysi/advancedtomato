/*
 * befs_fs.h
 *
 * Copyright (C) 2001-2002 Will Dyson <will_dyson@pobox.com>
 * Copyright (C) 1999 Makoto Kato (m_kato@ga2.so-net.ne.jp)
 */

#ifndef _LINUX_BEFS_FS
#define _LINUX_BEFS_FS

#include "befs_fs_types.h"
#include "compatibility.h"

/* used in debug.c */
#define BEFS_VERSION "0.9.2"

typedef __u64 befs_blocknr_t;
typedef __u32 vfs_blocknr_t;

/*
 * BeFS in memory structures
 */

typedef struct befs_mount_options {
	gid_t gid;
	uid_t uid;
	int use_gid;
	int use_uid;
	int debug;
	char *iocharset;
} befs_mount_options;

typedef struct befs_sb_info {
	__u32 magic1;
	__u32 block_size;
	__u32 block_shift;
	__u32 byte_order;
	befs_off_t num_blocks;
	befs_off_t used_blocks;
	__u32 inode_size;
	__u32 magic2;

	/* Allocation group information */
	__u32 blocks_per_ag;
	__u32 ag_shift;
	__u32 num_ags;

	/* jornal log entry */
	befs_block_run log_blocks;
	befs_off_t log_start;
	befs_off_t log_end;

	befs_inode_addr root_dir;
	befs_inode_addr indices;
	__u32 magic3;

	befs_mount_options mount_opts;
	struct nls_table *nls;

} befs_sb_info;

typedef struct befs_inode_info {
	__u32 i_flags;
	__u32 i_type;

	befs_inode_addr i_inode_num;
	befs_inode_addr i_parent;
	befs_inode_addr i_attribute;

	union {
		befs_data_stream ds;
		char symlink[BEFS_SYMLINK_LEN];
	} i_data;

} befs_inode_info;

enum befs_err {
	BEFS_OK,
	BEFS_ERR,
	BEFS_BAD_INODE,
	BEFS_BT_END,
	BEFS_BT_EMPTY,
	BEFS_BT_MATCH,
	BEFS_BT_PARMATCH,
	BEFS_BT_NOT_FOUND
};

/****************************/
/* io.c */
struct buffer_head *befs_bread_iaddr(struct super_block *sb,
				     befs_inode_addr iaddr);

struct buffer_head *befs_bread(struct super_block *sb, befs_blocknr_t block);
/****************************/

/****************************/
/* datastream.c */
struct buffer_head *befs_read_datastream(struct super_block *sb,
					 befs_data_stream * ds, befs_off_t pos,
					 uint * off);

int befs_fblock2brun(struct super_block *sb, befs_data_stream * data,
		     befs_blocknr_t fblock, befs_block_run * run);

size_t befs_read_lsymlink(struct super_block *sb, befs_data_stream * data,
			  void *buff, befs_off_t len);

befs_blocknr_t befs_count_blocks(struct super_block *sb, befs_data_stream * ds);

extern const befs_inode_addr BAD_IADDR;
/****************************/

/****************************/
/* debug.c */
void befs_error(const struct super_block *sb, const char *fmt, ...);
void befs_warning(const struct super_block *sb, const char *fmt, ...);
void befs_debug(const struct super_block *sb, const char *fmt, ...);

void befs_dump_super_block(const struct super_block *sb, befs_super_block *);
void befs_dump_inode(const struct super_block *sb, befs_inode *);
void befs_dump_index_entry(const struct super_block *sb, befs_btree_super *);
void befs_dump_index_node(const struct super_block *sb, befs_btree_nodehead *);
void befs_dump_inode_addr(const struct super_block *sb, befs_inode_addr);
/****************************/

/****************************/
/* btree.c */
int befs_btree_find(struct super_block *sb, befs_data_stream * ds,
		    const char *key, befs_off_t * value);

int befs_btree_read(struct super_block *sb, befs_data_stream * ds,
		    loff_t key_no, size_t bufsize, char *keybuf,
		    size_t * keysize, befs_off_t * value);
/****************************/

/****************************/
/* super.c */
int befs_load_sb(struct super_block *sb, befs_super_block * disk_sb);
int befs_check_sb(struct super_block *sb);
/****************************/

/****************************/
/* inode.c */
int befs_check_inode(struct super_block *sb, befs_inode * raw_inode,
		     befs_blocknr_t inode);
/****************************/

/* Gets a pointer to the private portion of the super_block
 * structure from the public part
 */
static inline befs_sb_info *
BEFS_SB(const struct super_block *super)
{
	return (befs_sb_info *) super->u.generic_sbp;
}

static inline befs_inode_info *
BEFS_I(const struct inode *inode)
{
	return (befs_inode_info *) inode->u.generic_ip;
}

static inline befs_blocknr_t
iaddr2blockno(struct super_block *sb, befs_inode_addr * iaddr)
{
	return ((iaddr->allocation_group << BEFS_SB(sb)->ag_shift) +
		iaddr->start);
}

static inline befs_inode_addr
blockno2iaddr(struct super_block *sb, befs_blocknr_t blockno)
{
	befs_inode_addr iaddr;
	iaddr.allocation_group = blockno >> BEFS_SB(sb)->ag_shift;
	iaddr.start =
	    blockno - (iaddr.allocation_group << BEFS_SB(sb)->ag_shift);
	iaddr.len = 1;

	return iaddr;
}

static inline unsigned int
befs_iaddrs_per_block(struct super_block *sb)
{
	return BEFS_SB(sb)->block_size / sizeof (befs_inode_addr);
}

static inline int
befs_iaddr_is_empty(befs_inode_addr * iaddr)
{
	return (!iaddr->allocation_group) && (!iaddr->start) && (!iaddr->len);
}

static inline size_t
befs_brun_size(struct super_block *sb, befs_block_run run)
{
	return BEFS_SB(sb)->block_size * run.len;
}

#endif				/* _LINUX_BEFS_FS */
