/*
 *   Copyright (c) International Business Machines Corp., 2000-2002
 *   Portions Copyright (c) Christoph Hellwig, 2001-2002
 *
 *   This program is free software;  you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or 
 *   (at your option) any later version.
 * 
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY;  without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 *   the GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program;  if not, write to the Free Software 
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <linux/fs.h>
#include <linux/init.h>
#include "jfs_incore.h"
#include "jfs_filsys.h"
#include "jfs_metapage.h"
#include "jfs_txnmgr.h"
#include "jfs_debug.h"

extern struct task_struct *jfsCommitTask;
static spinlock_t meta_lock = SPIN_LOCK_UNLOCKED;
static wait_queue_head_t meta_wait;

#ifdef CONFIG_JFS_STATISTICS
struct {
	uint	pagealloc;	/* # of page allocations */
	uint	pagefree;	/* # of page frees */
	uint	lockwait;	/* # of sleeping lock_metapage() calls */
	uint	allocwait;	/* # of sleeping alloc_metapage() calls */
} mpStat;
#endif


#define HASH_BITS 10		/* This makes hash_table 1 4K page */
#define HASH_SIZE (1 << HASH_BITS)
static struct metapage **hash_table = NULL;
static unsigned long hash_order;


static inline int metapage_locked(struct metapage *mp)
{
	return test_bit(META_locked, &mp->flag);
}

static inline int trylock_metapage(struct metapage *mp)
{
	return test_and_set_bit(META_locked, &mp->flag);
}

static inline void unlock_metapage(struct metapage *mp)
{
	clear_bit(META_locked, &mp->flag);
	wake_up(&mp->wait);
}

static void __lock_metapage(struct metapage *mp)
{
	DECLARE_WAITQUEUE(wait, current);

	INCREMENT(mpStat.lockwait);

	add_wait_queue_exclusive(&mp->wait, &wait);
	do {
		set_current_state(TASK_UNINTERRUPTIBLE);
		if (metapage_locked(mp)) {
			spin_unlock(&meta_lock);
			schedule();
			spin_lock(&meta_lock);
		}
	} while (trylock_metapage(mp));
	__set_current_state(TASK_RUNNING);
	remove_wait_queue(&mp->wait, &wait);
}

/* needs meta_lock */
static inline void lock_metapage(struct metapage *mp)
{
	if (trylock_metapage(mp))
		__lock_metapage(mp);
}

/*
 * metapage pool is based on Linux 2.5's mempool
 *
 * Tap into reserved structures in critical paths where waiting on a
 * memory allocation could cause deadlock
 */
#define METAPOOL_MIN_PAGES 32
static struct metapage *reserved_metapages[METAPOOL_MIN_PAGES];
static int num_reserved = 0;
kmem_cache_t *metapage_cache;

static void init_once(void *foo, kmem_cache_t *cachep, unsigned long flags)
{
	struct metapage *mp = (struct metapage *)foo;

	if ((flags & (SLAB_CTOR_VERIFY|SLAB_CTOR_CONSTRUCTOR)) ==
	    SLAB_CTOR_CONSTRUCTOR) {
		mp->lid = 0;
		mp->lsn = 0;
		mp->flag = 0;
		mp->data = NULL;
		mp->clsn = 0;
		mp->log = NULL;
		set_bit(META_free, &mp->flag);
		init_waitqueue_head(&mp->wait);
	}
}

static void empty_reserved(void)
{
	while (num_reserved--)
		kmem_cache_free(metapage_cache,
				reserved_metapages[num_reserved]);
}

static struct metapage *alloc_metapage(int *dropped_lock, int no_wait)
{
	struct metapage *new;

	*dropped_lock = 0;

	/*
	 * Always try an atomic alloc first, to avoid dropping the
	 * spinlock
	 */
	new = kmem_cache_alloc(metapage_cache, GFP_ATOMIC);
	if (new)
		return new;

	if (no_wait && num_reserved)
		return reserved_metapages[--num_reserved];

	*dropped_lock = 1;
	spin_unlock(&meta_lock);
	new = kmem_cache_alloc(metapage_cache, GFP_NOFS);
	spin_lock(&meta_lock);
	return new;
}

static void __free_metapage(struct metapage *mp)
{
	mp->flag = 0;
	set_bit(META_free, &mp->flag);

	if (num_reserved < METAPOOL_MIN_PAGES)
		reserved_metapages[num_reserved++] = mp;
	else
		kmem_cache_free(metapage_cache, mp);
}

static inline void free_metapage(struct metapage * mp)
{
	spin_lock(&meta_lock);
	__free_metapage(mp);
	spin_unlock(&meta_lock);
}

int __init metapage_init(void)
{
	struct metapage *mp;

	/*
	 * Initialize wait queue
	 */
	init_waitqueue_head(&meta_wait);

	/*
	 * Allocate the metapage structures
	 */
	metapage_cache = kmem_cache_create("jfs_mp", sizeof(struct metapage),
					   0, 0, init_once, NULL);
	if (metapage_cache == NULL)
		return -ENOMEM;

	while (num_reserved < METAPOOL_MIN_PAGES) {
		mp = kmem_cache_alloc(metapage_cache, GFP_NOFS);
		if (mp)
			reserved_metapages[num_reserved++] = mp;
		else {
			empty_reserved();
			kmem_cache_destroy(metapage_cache);
			return -ENOMEM;
		}
	}
	/*
	 * Now the hash list
	 */
	for (hash_order = 0;
	     ((PAGE_SIZE << hash_order) / sizeof(void *)) < HASH_SIZE;
	     hash_order++);
	hash_table =
	    (struct metapage **) __get_free_pages(GFP_KERNEL, hash_order);
	assert(hash_table);
	memset(hash_table, 0, PAGE_SIZE << hash_order);

	return 0;
}

void metapage_exit(void)
{
	empty_reserved();
	kmem_cache_destroy(metapage_cache);
}

/*
 * Basically same hash as in pagemap.h, but using our hash table
 */
static struct metapage **meta_hash(struct address_space *mapping,
				   unsigned long index)
{
#define i (((unsigned long)mapping)/ \
	   (sizeof(struct inode) & ~(sizeof(struct inode) -1 )))
#define s(x) ((x) + ((x) >> HASH_BITS))
	return hash_table + (s(i + index) & (HASH_SIZE - 1));
#undef i
#undef s
}

static struct metapage *search_hash(struct metapage ** hash_ptr,
				    struct address_space *mapping,
			       unsigned long index)
{
	struct metapage *ptr;

	for (ptr = *hash_ptr; ptr; ptr = ptr->hash_next) {
		if ((ptr->mapping == mapping) && (ptr->index == index))
			return ptr;
	}

	return NULL;
}

static void add_to_hash(struct metapage * mp, struct metapage ** hash_ptr)
{
	if (*hash_ptr)
		(*hash_ptr)->hash_prev = mp;

	mp->hash_prev = NULL;
	mp->hash_next = *hash_ptr;
	*hash_ptr = mp;
}

static void remove_from_hash(struct metapage * mp, struct metapage ** hash_ptr)
{
	if (mp->hash_prev)
		mp->hash_prev->hash_next = mp->hash_next;
	else {
		assert(*hash_ptr == mp);
		*hash_ptr = mp->hash_next;
	}

	if (mp->hash_next)
		mp->hash_next->hash_prev = mp->hash_prev;
}

struct metapage *__get_metapage(struct inode *inode, unsigned long lblock,
				unsigned int size, int absolute,
				unsigned long new)
{
	int dropped_lock;
	struct metapage **hash_ptr;
	int l2BlocksPerPage;
	int l2bsize;
	int no_wait;
	struct address_space *mapping;
	struct metapage *mp;
	unsigned long page_index;
	unsigned long page_offset;

	jFYI(1, ("__get_metapage: inode = 0x%p, lblock = 0x%lx\n",
		 inode, lblock));

	if (absolute)
		mapping = inode->i_sb->s_bdev->bd_inode->i_mapping;
	else
		mapping = inode->i_mapping;

	spin_lock(&meta_lock);

	hash_ptr = meta_hash(mapping, lblock);

	mp = search_hash(hash_ptr, mapping, lblock);
	if (mp) {
	      page_found:
		if (test_bit(META_discard, &mp->flag)) {
			assert(new);	/* It's okay to reuse a discarded
					 * if we expect it to be empty
					 */
			clear_bit(META_discard, &mp->flag);
		}
		mp->count++;
		jFYI(1, ("__get_metapage: found 0x%p, in hash\n", mp));
		assert(mp->logical_size == size);
		lock_metapage(mp);
		spin_unlock(&meta_lock);
	} else {
		l2bsize = inode->i_blkbits;
		l2BlocksPerPage = PAGE_CACHE_SHIFT - l2bsize;
		page_index = lblock >> l2BlocksPerPage;
		page_offset = (lblock - (page_index << l2BlocksPerPage)) <<
		    l2bsize;
		if ((page_offset + size) > PAGE_CACHE_SIZE) {
			spin_unlock(&meta_lock);
			jERROR(1, ("MetaData crosses page boundary!!\n"));
			return NULL;
		}
		
		/*
		 * Locks held on aggregate inode pages are usually
		 * not held long, and they are taken in critical code
		 * paths (committing dirty inodes, txCommit thread) 
		 * 
		 * Attempt to get metapage without blocking, tapping into
		 * reserves if necessary.
		 */
		if (JFS_IP(inode)->fileset == AGGREGATE_I)
			no_wait = 1;
		else
			no_wait = 0;

		mp = alloc_metapage(&dropped_lock, no_wait);
		if (dropped_lock) {
			/* alloc_metapage blocked, we need to search the hash
			 * again.
			 */
			struct metapage *mp2;
			mp2 = search_hash(hash_ptr, mapping, lblock);
			if (mp2) {
				__free_metapage(mp);
				mp = mp2;
				goto page_found;
			}
		}
		mp->flag = 0;
		lock_metapage(mp);
		if (absolute)
			set_bit(META_absolute, &mp->flag);
		mp->xflag = COMMIT_PAGE;
		mp->count = 1;
		atomic_set(&mp->nohomeok,0);
		mp->mapping = mapping;
		mp->index = lblock;
		mp->page = 0;
		mp->logical_size = size;
		add_to_hash(mp, hash_ptr);
		if (!absolute)
			list_add(&mp->inode_list, &JFS_IP(inode)->mp_list);
		spin_unlock(&meta_lock);

		if (new) {
			jFYI(1,
			     ("__get_metapage: Calling grab_cache_page\n"));
			mp->page = grab_cache_page(mapping, page_index);
			if (!mp->page) {
				jERROR(1, ("grab_cache_page failed!\n"));
				goto freeit;
			} else
				INCREMENT(mpStat.pagealloc);
		} else {
			jFYI(1,
			     ("__get_metapage: Calling read_cache_page\n"));
			mp->page = read_cache_page(mapping, lblock,
				    (filler_t *)mapping->a_ops->readpage, NULL);
			if (IS_ERR(mp->page)) {
				jERROR(1, ("read_cache_page failed!\n"));
				goto freeit;
			} else
				INCREMENT(mpStat.pagealloc);
			lock_page(mp->page);
		}
		mp->data = kmap(mp->page) + page_offset;
	}
	jFYI(1, ("__get_metapage: returning = 0x%p\n", mp));
	return mp;

freeit:
	spin_lock(&meta_lock);
	remove_from_hash(mp, hash_ptr);
	if (!absolute)
		list_del(&mp->inode_list);
	__free_metapage(mp);
	spin_unlock(&meta_lock);
	return NULL;
}

void hold_metapage(struct metapage * mp, int force)
{
	spin_lock(&meta_lock);

	mp->count++;

	if (force) {
		ASSERT (!(test_bit(META_forced, &mp->flag)));
		if (trylock_metapage(mp))
			set_bit(META_forced, &mp->flag);
	} else
		lock_metapage(mp);

	spin_unlock(&meta_lock);
}

static void __write_metapage(struct metapage * mp)
{
	int l2bsize = mp->mapping->host->i_blkbits;
	int l2BlocksPerPage = PAGE_CACHE_SHIFT - l2bsize;
	unsigned long page_index;
	unsigned long page_offset;
	int rc;

	jFYI(1, ("__write_metapage: mp = 0x%p\n", mp));

	if (test_bit(META_discard, &mp->flag)) {
		/*
		 * This metadata is no longer valid
		 */
		clear_bit(META_dirty, &mp->flag);
		return;
	}

	page_index = mp->page->index;
	page_offset =
	    (mp->index - (page_index << l2BlocksPerPage)) << l2bsize;

	rc = mp->mapping->a_ops->prepare_write(NULL, mp->page, page_offset,
					       page_offset +
					       mp->logical_size);
	if (rc) {
		jERROR(1, ("prepare_write return %d!\n", rc));
		ClearPageUptodate(mp->page);
		kunmap(mp->page);
		clear_bit(META_dirty, &mp->flag);
		return;
	}
	rc = mp->mapping->a_ops->commit_write(NULL, mp->page, page_offset,
					      page_offset +
					      mp->logical_size);
	if (rc) {
		jERROR(1, ("commit_write returned %d\n", rc));
	}

	clear_bit(META_dirty, &mp->flag);

	jFYI(1, ("__write_metapage done\n"));
}

static inline void sync_metapage(struct metapage *mp)
{
	struct page *page = mp->page;

	page_cache_get(page);
	lock_page(page);

	/* we're done with this page - no need to check for errors */
	if (page->buffers) {
		writeout_one_page(page);
		waitfor_one_page(page);
	}

	UnlockPage(page);
	page_cache_release(page);
}

void release_metapage(struct metapage * mp)
{
	struct jfs_log *log;

	jFYI(1,
	     ("release_metapage: mp = 0x%p, flag = 0x%lx\n", mp,
	      mp->flag));

	spin_lock(&meta_lock);
	if (test_bit(META_forced, &mp->flag)) {
		clear_bit(META_forced, &mp->flag);
		mp->count--;
		spin_unlock(&meta_lock);
		return;
	}

	assert(mp->count);
	if (--mp->count || atomic_read(&mp->nohomeok)) {
		unlock_metapage(mp);
		spin_unlock(&meta_lock);
	} else {
		remove_from_hash(mp, meta_hash(mp->mapping, mp->index));
		if (!test_bit(META_absolute, &mp->flag))
			list_del(&mp->inode_list);
		spin_unlock(&meta_lock);

		if (mp->page) {
			kunmap(mp->page);
			mp->data = 0;
			if (test_bit(META_dirty, &mp->flag))
				__write_metapage(mp);
			UnlockPage(mp->page);
			if (test_bit(META_sync, &mp->flag)) {
				sync_metapage(mp);
				clear_bit(META_sync, &mp->flag);
			}

			if (test_bit(META_discard, &mp->flag)) {
				lock_page(mp->page);
				block_flushpage(mp->page, 0);
				unlock_page(mp->page);
			}

			page_cache_release(mp->page);
			INCREMENT(mpStat.pagefree);
		}

		if (mp->lsn) {
			/*
			 * Remove metapage from logsynclist.
			 */
			log = mp->log;
			LOGSYNC_LOCK(log);
			mp->log = 0;
			mp->lsn = 0;
			mp->clsn = 0;
			log->count--;
			list_del(&mp->synclist);
			LOGSYNC_UNLOCK(log);
		}

		free_metapage(mp);
	}
	jFYI(1, ("release_metapage: done\n"));
}

void __invalidate_metapages(struct inode *ip, s64 addr, int len)
{
	struct metapage **hash_ptr;
	unsigned long lblock;
	int l2BlocksPerPage = PAGE_CACHE_SHIFT - ip->i_blkbits;
	struct address_space *mapping = ip->i_mapping;
	struct metapage *mp;
	struct page *page;

	/*
	 * First, mark metapages to discard.  They will eventually be
	 * released, but should not be written.
	 */
	for (lblock = addr; lblock < addr + len;
	     lblock += 1 << l2BlocksPerPage) {
		hash_ptr = meta_hash(mapping, lblock);
		spin_lock(&meta_lock);
		mp = search_hash(hash_ptr, mapping, lblock);
		if (mp) {
			set_bit(META_discard, &mp->flag);
			spin_unlock(&meta_lock);
			/*
			 * If in the metapage cache, we've got the page locked
			 */
			block_flushpage(mp->page, 0);
		} else {
			spin_unlock(&meta_lock);
			page = find_lock_page(mapping, lblock>>l2BlocksPerPage);
			if (page) {
				block_flushpage(page, 0);
				UnlockPage(page);
			}
		}
	}
}

void invalidate_inode_metapages(struct inode *inode)
{
	struct list_head *ptr;
	struct metapage *mp;

	spin_lock(&meta_lock);
	list_for_each(ptr, &JFS_IP(inode)->mp_list) {
		mp = list_entry(ptr, struct metapage, inode_list);
		clear_bit(META_dirty, &mp->flag);
		set_bit(META_discard, &mp->flag);
		kunmap(mp->page);
		UnlockPage(mp->page);
		page_cache_release(mp->page);
		INCREMENT(mpStat.pagefree);
		mp->data = 0;
		mp->page = 0;
	}
	spin_unlock(&meta_lock);
	truncate_inode_pages(inode->i_mapping, 0);
}

#ifdef CONFIG_JFS_STATISTICS
int jfs_mpstat_read(char *buffer, char **start, off_t offset, int length,
		    int *eof, void *data)
{
	int len = 0;
	off_t begin;

	len += sprintf(buffer,
		       "JFS Metapage statistics\n"
		       "=======================\n"
		       "page allocations = %d\n"
		       "page frees = %d\n"
		       "lock waits = %d\n"
		       "allocation waits = %d\n",
		       mpStat.pagealloc,
		       mpStat.pagefree,
		       mpStat.lockwait,
		       mpStat.allocwait);

	begin = offset;
	*start = buffer + begin;
	len -= begin;

	if (len > length)
		len = length;
	else
		*eof = 1;

	if (len < 0)
		len = 0;

	return len;
}
#endif
