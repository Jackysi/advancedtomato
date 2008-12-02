/*
 * linux/fs/nfsacl.c
 *
 *  Copyright (C) 2002 by Andreas Gruenbacher <a.gruenbacher@computer.org>
 */

/*
 * The Solaris nfsacl protocol represents some ACLs slightly differently
 * than POSIX 1003.1e draft 17 does (and we do):
 *
 *  - Minimal ACLs always have an ACL_MASK entry, so they have
 *    four instead of three entries.
 *  - The ACL_MASK entry in such minimal ACLs always has the same
 *    permissions as the ACL_GROUP_OBJ entry. (In extended ACLs
 *    the ACL_MASK and ACL_GROUP_OBJ entries may differ.)
 *  - The identifier fields of the ACL_USER_OBJ and ACL_GROUP_OBJ
 *    entries contain the identifiers of the owner and owning group.
 *    (In POSIX ACLs we always set them to ACL_UNDEFINED_ID).
 *  - ACL entries in the kernel are kept sorted in ascending order
 *    of (e_tag, e_id). Solaris ACLs are unsorted.
 */

#include <linux/module.h>
#include <linux/nfsacl.h>
#include <linux/nfs3.h>

EXPORT_SYMBOL(nfsacl_encode);
EXPORT_SYMBOL(nfsacl_decode);

u32 *
nfsacl_encode(u32 *p, u32 *end, struct inode *inode, struct posix_acl *acl,
	      int encode_entries, int typeflag)
{
	int entries = acl ? acl->a_count : 0;
	
	if (entries == 3)
		entries++;  /* need to fake up ACL_MASK entry */
	if (entries > NFS3_ACL_MAX_ENTRIES ||
	    p + 2 + (encode_entries ? (3 * entries) : 0) > end)
		return NULL;
	*p++ = htonl(entries);
	if (acl && encode_entries) {
		struct posix_acl_entry *pa, *pe;
		int group_obj_perm = ACL_READ|ACL_WRITE|ACL_EXECUTE;

		*p++ = htonl(entries);
		FOREACH_ACL_ENTRY(pa, acl, pe) {
			*p++ = htonl(pa->e_tag | typeflag);
			switch(pa->e_tag) {
				case ACL_USER_OBJ:
					*p++ = htonl(inode->i_uid);
					break;
				case ACL_GROUP_OBJ:
					*p++ = htonl(inode->i_gid);
					group_obj_perm = pa->e_perm;
					break;
				case ACL_USER:
				case ACL_GROUP:
					*p++ = htonl(pa->e_id);
					break;
				default:  /* Solaris depends on that! */
					*p++ = 0;
					break;
			}
			*p++ = htonl(pa->e_perm & S_IRWXO);
		}
		if (acl->a_count < entries) {
			/* fake up ACL_MASK entry */
			*p++ = htonl(ACL_MASK | typeflag);
			*p++ = htonl(ACL_UNDEFINED_ID);
			*p++ = htonl(group_obj_perm & S_IRWXO);
		}
	} else
		*p++ = 0;

	return p;
}

static int
cmp_acl_entry(const struct posix_acl_entry *a, const struct posix_acl_entry *b)
{
	if (a->e_tag != b->e_tag)
		return a->e_tag - b->e_tag;
	else if (a->e_id > b->e_id)
		return 1;
	else if (a->e_id < b->e_id)
		return -1;
	else
		return 0;
}

/*
 * Convert from a Solaris ACL to a POSIX 1003.1e draft 17 ACL.
 */
static int
posix_acl_from_nfsacl(struct posix_acl *acl)
{
	struct posix_acl_entry *pa, *pe,
	       *group_obj = NULL, *mask = NULL;

	if (!acl)
		return 0;

	qsort(acl->a_entries, acl->a_count, sizeof(struct posix_acl_entry),
	      (int(*)(const void *,const void *))cmp_acl_entry);

	/* Clear undefined identifier fields and find the ACL_GROUP_OBJ
	   and ACL_MASK entries. */
	FOREACH_ACL_ENTRY(pa, acl, pe) {
		switch(pa->e_tag) {
			case ACL_USER_OBJ:
				pa->e_id = ACL_UNDEFINED_ID;
				break;
			case ACL_GROUP_OBJ:
				pa->e_id = ACL_UNDEFINED_ID;
				group_obj = pa;
				break;
			case ACL_MASK:
				mask = pa;
				/* fall through */
			case ACL_OTHER:
				pa->e_id = ACL_UNDEFINED_ID;
				break;
		}
	}
	if (acl->a_count == 4 && group_obj && mask &&
	    mask->e_perm == group_obj->e_perm) {
		/* remove bogus ACL_MASK entry */
		memmove(mask, mask+1, (acl->a_entries + 4 - mask) *
				      sizeof(struct posix_acl_entry));
		acl->a_count = 3;
	}
	return 0;
}

static u32 *
nfsacl_decode_entry(u32 *p, struct posix_acl_entry *entry)
{
	entry->e_tag = ntohl(*p++) & ~NFS3_ACL_DEFAULT;
	entry->e_id = ntohl(*p++);
	entry->e_perm = ntohl(*p++);

	switch(entry->e_tag) {
		case ACL_USER_OBJ:
		case ACL_USER:
		case ACL_GROUP_OBJ:
		case ACL_GROUP:
		case ACL_OTHER:
			if (entry->e_perm & ~S_IRWXO)
				return NULL;
			break;
		case ACL_MASK:
			/* Solaris sometimes sets additonal bits in the mask */
			entry->e_perm &= S_IRWXO;
			break;
		default:
			return NULL;
	}
	return p;
}

u32 *
nfsacl_decode(u32 *p, u32 *end, unsigned int *aclcnt, struct posix_acl **pacl)
{
	struct posix_acl_entry *pa, *pe;
	unsigned int entries, array_len;

	if (p + 2 > end)
		return NULL;
	entries = ntohl(*p++);
	array_len = ntohl(*p++);
	if (entries > NFS3_ACL_MAX_ENTRIES || (pacl && entries != array_len))
		return NULL;
	if (p + 3 * array_len > end)
		return NULL;
	if (pacl) {
		*pacl = NULL;
		if (entries) {
			struct posix_acl *acl;
			
			if (!(acl = posix_acl_alloc(array_len, GFP_KERNEL)))
				return NULL;
			FOREACH_ACL_ENTRY(pa, acl, pe) {
				if (!(p = nfsacl_decode_entry(p, pa))) {
					posix_acl_release(acl);
					return NULL;
				}
			}
			if (posix_acl_from_nfsacl(acl) != 0) {
				posix_acl_release(acl);
				return NULL;
			}
			*pacl = acl;
		}
	} else 
		p += 3 * array_len;

	if (aclcnt)
		*aclcnt = entries;
	return p;
}

