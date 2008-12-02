/*
  File: linux/reiserfs_xattr.h
*/

#include <linux/config.h>
#include <linux/init.h>
#include <linux/xattr.h>

/* Magic value in header */
#define REISERFS_XATTR_MAGIC 0x52465841 /* "RFXA" */

struct reiserfs_xattr_header {
    __u32 h_magic;              /* magic number for identification */
    __u32 h_hash;               /* hash of the value */
};

#ifdef __KERNEL__

struct reiserfs_xattr_handler {
	char *prefix;
	int (*get)(struct inode *inode, const char *name, void *buffer,
		   size_t size);
	int (*set)(struct inode *inode, const char *name, const void *buffer,
		   size_t size, int flags);
	int (*del)(struct inode *inode, const char *name);
        int (*list)(struct inode *inode, const char *name, int namelen, char *out);
        struct reiserfs_xattr_handler *next;
};


#ifdef CONFIG_REISERFS_FS_XATTR
#define is_reiserfs_priv_object(inode) (((inode)->u.reiserfs_i.i_flags & i_priv_object) == i_priv_object)
ssize_t reiserfs_getxattr (struct dentry *dentry, const char *name,
			   void *buffer, size_t size);
int reiserfs_setxattr (struct dentry *dentry, const char *name,
                       const void *value, size_t size, int flags);
ssize_t reiserfs_listxattr (struct dentry *dentry, char *buffer, size_t size);
int reiserfs_removexattr (struct dentry *dentry, const char *name);
int reiserfs_delete_xattrs (struct inode *inode);
int reiserfs_chown_xattrs (struct inode *inode, struct iattr *attrs);
int reiserfs_xattr_init (struct super_block *sb, int mount_flags);

static inline void
reiserfs_write_lock_xattrs(struct super_block *sb)
{
    down_write (&REISERFS_XATTR_DIR_SEM(sb));
}
static inline void
reiserfs_write_unlock_xattrs(struct super_block *sb)
{
    up_write (&REISERFS_XATTR_DIR_SEM(sb));
}
static inline void
reiserfs_read_lock_xattrs(struct super_block *sb)
{
    down_read (&REISERFS_XATTR_DIR_SEM(sb));
}
static inline void
reiserfs_read_unlock_xattrs(struct super_block *sb)
{
    up_read (&REISERFS_XATTR_DIR_SEM(sb));
}
#else
#define is_reiserfs_priv_object(inode) 0
#define reiserfs_getxattr NULL
#define reiserfs_setxattr NULL
#define reiserfs_listxattr NULL
#define reiserfs_removexattr NULL
#define reiserfs_write_lock_xattrs(sb)
#define reiserfs_write_unlock_xattrs(sb)
#define reiserfs_read_lock_xattrs(sb)
#define reiserfs_read_unlock_xattrs(sb)
static inline int
reiserfs_xattr_init (struct super_block *s, int mount_flags)
{
    s->s_flags = (s->s_flags & ~MS_POSIXACL); /* to be sure */
    return 0;
}

static inline int
reiserfs_delete_xattrs (struct inode *inode)
{
    return 0;
}

static inline int
reiserfs_chown_xattrs (struct inode *inode, struct iattr *attrs)
{
    return 0;
}
#endif

extern int reiserfs_xattr_register_handler(struct reiserfs_xattr_handler *);
extern int reiserfs_xattr_unregister_handler(struct reiserfs_xattr_handler *);
extern int reiserfs_xattr_del (struct inode *, const char *);
extern int reiserfs_xattr_get (const struct inode *, const char *, void *, size_t);
extern int reiserfs_xattr_set (struct inode *, const char *, const void *,
                               size_t, int);
#ifdef CONFIG_REISERFS_FS_XATTR_USER
extern int reiserfs_xattr_user_init (void) __init;
extern int reiserfs_xattr_user_exit (void);
#else
static inline int
reiserfs_xattr_user_init (void)
{
    return 0;
}

static inline int
reiserfs_xattr_user_exit (void)
{
    return 0;
}
#endif
#ifdef CONFIG_REISERFS_FS_XATTR_TRUSTED
extern int reiserfs_xattr_trusted_init (void) __init;
extern int reiserfs_xattr_trusted_exit (void);
#else
static inline int
reiserfs_xattr_trusted_init (void)
{
    return 0;
}

static inline int
reiserfs_xattr_trusted_exit (void)
{
    return 0;
}
#endif

#endif  /* __KERNEL__ */
