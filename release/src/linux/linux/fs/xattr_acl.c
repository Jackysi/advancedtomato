/*
 * linux/fs/xattr_acl.c
 *
 * Almost all from linux/fs/ext2/acl.c:
 * Copyright (C) 2001 by Andreas Gruenbacher, <a.gruenbacher@computer.org>
 */

#include <linux/module.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/xattr_acl.h>


/*
 * Convert from extended attribute to in-memory representation.
 */
struct posix_acl *
posix_acl_from_xattr(const void *value, size_t size)
{
	xattr_acl_header *header = (xattr_acl_header *)value;
	xattr_acl_entry *entry = (xattr_acl_entry *)(header+1), *end;
	int count;
	struct posix_acl *acl;
	struct posix_acl_entry *acl_e;

	if (!value)
		return NULL;
	if (size < sizeof(xattr_acl_header))
		 return ERR_PTR(-EINVAL);
	if (header->a_version != cpu_to_le32(XATTR_ACL_VERSION))
		return ERR_PTR(-EINVAL);

	count = xattr_acl_count(size);
	if (count < 0)
		return ERR_PTR(-EINVAL);
	if (count == 0)
		return NULL;
	
	acl = posix_acl_alloc(count, GFP_KERNEL);
	if (!acl)
		return ERR_PTR(-ENOMEM);
	acl_e = acl->a_entries;
	
	for (end = entry + count; entry != end; acl_e++, entry++) {
		acl_e->e_tag  = le16_to_cpu(entry->e_tag);
		acl_e->e_perm = le16_to_cpu(entry->e_perm);

		switch(acl_e->e_tag) {
			case ACL_USER_OBJ:
			case ACL_GROUP_OBJ:
			case ACL_MASK:
			case ACL_OTHER:
				acl_e->e_id = ACL_UNDEFINED_ID;
				break;

			case ACL_USER:
			case ACL_GROUP:
				acl_e->e_id = le32_to_cpu(entry->e_id);
				break;

			default:
				goto fail;
		}
	}
	return acl;

fail:
	posix_acl_release(acl);
	return ERR_PTR(-EINVAL);
}
EXPORT_SYMBOL (posix_acl_from_xattr);

/*
 * Convert from in-memory to extended attribute representation.
 */
int
posix_acl_to_xattr(const struct posix_acl *acl, void *buffer, size_t size)
{
	xattr_acl_header *ext_acl = (xattr_acl_header *)buffer;
	xattr_acl_entry *ext_entry = ext_acl->a_entries;
	int real_size, n;

	real_size = xattr_acl_size(acl->a_count);
	if (!buffer)
		return real_size;
	if (real_size > size)
		return -ERANGE;
	
	ext_acl->a_version = cpu_to_le32(XATTR_ACL_VERSION);

	for (n=0; n < acl->a_count; n++, ext_entry++) {
		ext_entry->e_tag  = cpu_to_le16(acl->a_entries[n].e_tag);
		ext_entry->e_perm = cpu_to_le16(acl->a_entries[n].e_perm);
		ext_entry->e_id   = cpu_to_le32(acl->a_entries[n].e_id);
	}
	return real_size;
}
EXPORT_SYMBOL (posix_acl_to_xattr);
