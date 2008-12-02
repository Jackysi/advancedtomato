/*
  File: linux/posix_acl_xattr.h

  Extended attribute system call representation of Access Control Lists.

  Copyright (C) 2000 by Andreas Gruenbacher <a.gruenbacher@computer.org>
 */
#ifndef _POSIX_ACL_XATTR_H
#define _POSIX_ACL_XATTR_H

/* Extended attribute names */
#define POSIX_ACL_XATTR_ACCESS	"system.posix_acl_access"
#define POSIX_ACL_XATTR_DEFAULT	"system.posix_acl_default"

/* Supported ACL a_version fields */
#define POSIX_ACL_XATTR_VERSION	0x0002


/* An undefined entry e_id value */
#define ACL_UNDEFINED_ID	(-1)

/* ACL entry e_tag field values */
#define ACL_USER_OBJ		(0x01)
#define ACL_USER		(0x02)
#define ACL_GROUP_OBJ		(0x04)
#define ACL_GROUP		(0x08)
#define ACL_MASK		(0x10)
#define ACL_OTHER		(0x20)

/* ACL entry e_perm bitfield values */
#define ACL_READ		(0x04)
#define ACL_WRITE		(0x02)
#define ACL_EXECUTE		(0x01)


typedef struct {
	__u16			e_tag;
	__u16			e_perm;
	__u32			e_id;
} posix_acl_xattr_entry;

typedef struct {
	__u32			a_version;
	posix_acl_xattr_entry	a_entries[0];
} posix_acl_xattr_header;


static inline size_t
posix_acl_xattr_size(int count)
{
	return (sizeof(posix_acl_xattr_header) +
		(count * sizeof(posix_acl_xattr_entry)));
}

static inline int
posix_acl_xattr_count(size_t size)
{
	if (size < sizeof(posix_acl_xattr_header))
		return -1;
	size -= sizeof(posix_acl_xattr_header);
	if (size % sizeof(posix_acl_xattr_entry))
		return -1;
	return size / sizeof(posix_acl_xattr_entry);
}

#endif	/* _POSIX_ACL_XATTR_H */
