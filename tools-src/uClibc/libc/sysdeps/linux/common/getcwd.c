/* These functions find the absolute path to the current working directory.  */

#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <sys/syscall.h>

#ifdef __NR_getcwd

#ifndef INLINE_SYSCALL
#define INLINE_SYSCALL(name, nr, args...) __syscall_getcwd (args)
#define __NR___syscall_getcwd __NR_getcwd
static inline _syscall2(int, __syscall_getcwd, char *, buf, unsigned long, size);
#endif

char *getcwd(char *buf, int size)
{
    int olderrno, ret;
    char *allocbuf;

    if (size == 0) {
	if (buf != NULL) {
	    __set_errno(EINVAL);
	    return NULL;
	}
	size = PATH_MAX;
    }
    if (size < 3) {
	__set_errno(ERANGE);
	return NULL;
    }
    allocbuf=NULL;
    olderrno = errno;
    if (buf == NULL) {
	buf = allocbuf = malloc (size);
	if (buf == NULL)
	    return NULL;
    }
    ret = INLINE_SYSCALL(getcwd, 2, buf, size);
    if (ret < 0) {
	if (allocbuf) {
	    free(allocbuf);
	}
	return NULL;
    } 
    __set_errno(olderrno);
    return buf;
}
#else

/* If the syscall is not present, we have to walk up the
 * directory tree till we hit the root.  Now we _could_
 * use /proc/self/cwd if /proc is mounted... That approach
 * is left an an exercise for the reader... */


/* Seems a few broken filesystems (like coda) don't like this */
/* #undef FAST_DIR_SEARCH_POSSIBLE on Linux */


/* Routine to find the step back down */
static char *search_dir(dev_t this_dev, ino_t this_ino, char *path_buf, int path_size)
{
	DIR *dp;
	struct dirent *d;
	char *ptr;
	int slen;
	struct stat st;

#ifdef FAST_DIR_SEARCH_POSSIBLE
	/* The test is for ELKS lib 0.0.9, this should be fixed in the real kernel */
	int slow_search = (sizeof(ino_t) != sizeof(d->d_ino));
#endif

	if (stat(path_buf, &st) < 0)
		return 0;
#ifdef FAST_DIR_SEARCH_POSSIBLE
	if (this_dev != st.st_dev)
		slow_search = 1;
#endif

	slen = strlen(path_buf);
	ptr = path_buf + slen - 1;
	if (*ptr != '/') {
		if (slen + 2 > path_size) {
			__set_errno(ERANGE);
			return 0;
		}
		strcpy(++ptr, "/");
		slen++;
	}
	slen++;

	dp = opendir(path_buf);
	if (dp == 0)
		return 0;

	while ((d = readdir(dp)) != 0) {
#ifdef FAST_DIR_SEARCH_POSSIBLE
		if (slow_search || this_ino == d->d_ino) {
#endif
			if (slen + strlen(d->d_name) > path_size) {
				__set_errno(ERANGE);
				return 0;
			}
			strcpy(ptr + 1, d->d_name);
			if (stat(path_buf, &st) < 0)
				continue;
			if (st.st_ino == this_ino && st.st_dev == this_dev) {
				closedir(dp);
				return path_buf;
			}
#ifdef FAST_DIR_SEARCH_POSSIBLE
		}
#endif
	}

	closedir(dp);
	__set_errno(ENOENT);
	return 0;
}

/* Routine to go up tree */
static char *recurser(char *path_buf, int path_size, dev_t root_dev, ino_t root_ino)
{
	struct stat st;
	dev_t this_dev;
	ino_t this_ino;

	if (stat(path_buf, &st) < 0)
		return 0;
	this_dev = st.st_dev;
	this_ino = st.st_ino;
	if (this_dev == root_dev && this_ino == root_ino) {
		strcpy(path_buf, "/");
		return path_buf;
	}
	if (strlen(path_buf) + 4 > path_size) {
		__set_errno(ERANGE);
		return 0;
	}
	strcat(path_buf, "/..");
	if (recurser(path_buf, path_size, root_dev, root_ino) == 0)
		return 0;

	return search_dir(this_dev, this_ino, path_buf, path_size);
}


char *getcwd(char *buf, int size)
{
    struct stat st;

    if (size == 0) {
	if (buf != NULL) {
	    __set_errno(EINVAL);
	    return NULL;
	}
	size = PATH_MAX;
    }
    if (size < 3) {
	__set_errno(ERANGE);
	return NULL;
    }

    if (buf == NULL) {
	buf = malloc (size);
	if (buf == NULL)
	    return NULL;
    }

    strcpy(buf, ".");
    if (stat("/", &st) < 0) {
	return NULL;
    }

    return recurser(buf, size, st.st_dev, st.st_ino);
}

#endif
