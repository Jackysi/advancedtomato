/*
  File: linux/nfsacl.h

  (C) 2002 Andreas Gruenbacher, <a.gruenbacher@computer.org>
*/


#ifndef __LINUX_SOLARIS_ACL_H
#define __LINUX_SOLARIS_ACL_H

#include <linux/posix_acl.h>

u32 *nfsacl_encode(u32 *, u32 *, struct inode *, struct posix_acl *, int, int);
u32 *nfsacl_decode(u32 *, u32 *, unsigned int *, struct posix_acl **);

#endif  /* __LINUX_SOLARIS_ACL_H */
