/*
	Copyright 2005, Broadcom Corporation
	All Rights Reserved.

	THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
	KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
	SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
	FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
	
*/

# ifndef _GNU_SOURCE
#  define _GNU_SOURCE
# endif
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/time.h>
//	#include <net/ethernet.h>
#include <syslog.h>

#include <wlioctl.h>
#include <bcmnvram.h>

#include "shutils.h"
#include "shared.h"

/*
 * Concatenates NULL-terminated list of arguments into a single
 * commmand and executes it
 * @param	argv	argument list
 * @param	path	NULL, ">output", or ">>output"
 * @param	timeout	seconds to wait before timing out or 0 for no timeout
 * @param	ppid	NULL to wait for child termination or pointer to pid
 * @return	return value of executed command or errno
 *
 * Ref: http://www.open-std.org/jtc1/sc22/WG15/docs/rr/9945-2/9945-2-28.html
 */
int _eval(char *const argv[], const char *path, int timeout, int *ppid)
{
	sigset_t set, sigmask;
	sighandler_t chld = SIG_IGN;
	pid_t pid, w;
	int status = 0;
	int fd;
	int flags;
	int sig;
	int n;
	const char *p;
	char s[256];

	if (!ppid) {
		// block SIGCHLD
		sigemptyset(&set);
		sigaddset(&set, SIGCHLD);
		sigprocmask(SIG_BLOCK, &set, &sigmask);
		// without this we cannot rely on waitpid() to tell what happened to our children
		chld = signal(SIGCHLD, SIG_DFL);
	}

	pid = fork();
	if (pid == -1) {
		perror("fork");
		status = errno;
		goto EXIT;
	}
	if (pid != 0) {
		// parent
		if (ppid) {
			*ppid = pid;
			return 0;
		}
		do {
			if ((w = waitpid(pid, &status, 0)) == -1) {
				status = errno;
				perror("waitpid");
				goto EXIT;
			}
		} while (!WIFEXITED(status) && !WIFSIGNALED(status));

		if (WIFEXITED(status)) status = WEXITSTATUS(status);
EXIT:
		if (!ppid) {
			// restore signals
			sigprocmask(SIG_SETMASK, &sigmask, NULL);
			signal(SIGCHLD, chld);
			// reap zombies
			chld_reap(0);
		}
		return status;
	}
	
	// child

	// reset signal handlers
	for (sig = 0; sig < (_NSIG - 1); sig++)
		signal(sig, SIG_DFL);

	// unblock signals if called from signal handler
	sigemptyset(&set);
	sigprocmask(SIG_SETMASK, &set, NULL);

	setsid();

	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);
	open("/dev/null", O_RDONLY);
	open("/dev/null", O_WRONLY);
	open("/dev/null", O_WRONLY);

	if (nvram_match("debug_logeval", "1")) {
		pid = getpid();

		cprintf("_eval +%ld pid=%d ", get_uptime(), pid);
		for (n = 0; argv[n]; ++n) cprintf("%s ", argv[n]);
		cprintf("\n");
		
		if ((fd = open("/dev/console", O_RDWR)) >= 0) {
			dup2(fd, STDIN_FILENO);
			dup2(fd, STDOUT_FILENO);
			dup2(fd, STDERR_FILENO);
		}
		else {
			sprintf(s, "/tmp/eval.%d", pid);
			if ((fd = open(s, O_CREAT|O_RDWR, 0600)) >= 0) {
				dup2(fd, STDOUT_FILENO);
				dup2(fd, STDERR_FILENO);
			}
		}
		if (fd > STDERR_FILENO) close(fd);
	}

	// Redirect stdout & stderr to <path>
	if (path) {
		flags = O_WRONLY | O_CREAT;
		if (*path == '>') {
			++path;
			if (*path == '>') {
				++path;
				// >>path, append
				flags |= O_APPEND;
			}
			else {
				// >path, overwrite
				flags |= O_TRUNC;
			}
		}
		
		if ((fd = open(path, flags, 0644)) < 0) {
			perror(path);
		}
		else {
			dup2(fd, STDOUT_FILENO);
			dup2(fd, STDERR_FILENO);
			close(fd);
		}
	}

	// execute command

	p = nvram_safe_get("env_path");
	snprintf(s, sizeof(s), "%s%s/sbin:/bin:/usr/sbin:/usr/bin:/opt/sbin:/opt/bin", *p ? p : "", *p ? ":" : "");
	setenv("PATH", s, 1);

	alarm(timeout);
	execvp(argv[0], argv);
	
	perror(argv[0]);
	_exit(errno);
}

/*
 * fread() with automatic retry on syscall interrupt
 * @param	ptr	location to store to
 * @param	size	size of each element of data
 * @param	nmemb	number of elements
 * @param	stream	file stream
 * @return	number of items successfully read
 */
int safe_fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	size_t ret = 0;
	do {
		clearerr(stream);
		ret += fread((char *)ptr + (ret * size), size, nmemb - ret, stream);
	} while (ret < nmemb && ferror(stream) && errno == EINTR);
	return ret;
}

/*
 * fwrite() with automatic retry on syscall interrupt
 * @param	ptr	location to read from
 * @param	size	size of each element of data
 * @param	nmemb	number of elements
 * @param	stream	file stream
 * @return	number of items successfully written
 */
int safe_fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	size_t ret = 0;
	do {
		clearerr(stream);
		ret += fwrite((char *)ptr + (ret * size), size, nmemb - ret, stream);
	} while (ret < nmemb && ferror(stream) && errno == EINTR);

	return ret;
}

/*
 * Convert Ethernet address string representation to binary data
 * @param	a	string in xx:xx:xx:xx:xx:xx notation
 * @param	e	binary data
 * @return	TRUE if conversion was successful and FALSE otherwise
 */
int ether_atoe(const char *a, unsigned char *e)
{
	char *x;
	int i;

	i = 0;
	while (1) {
		e[i++] = (unsigned char) strtoul(a, &x, 16);
		if (a == x) break;
		if (i == ETHER_ADDR_LEN) return 1;
		if (*x == 0) break;
		a = x + 1;
	}
	memset(e, 0, sizeof(e));
	return 0;
}

/*
 * Convert Ethernet address binary data to string representation
 * @param	e	binary data
 * @param	a	string in xx:xx:xx:xx:xx:xx notation
 * @return	a
 */
char *ether_etoa(const unsigned char *e, char *a)
{
	sprintf(a, "%02X:%02X:%02X:%02X:%02X:%02X", e[0], e[1], e[2], e[3], e[4], e[5]);
	return a;
}

void cprintf(const char *format, ...)
{
	FILE *f;
	va_list args;

#ifdef DEBUG_NOISY
	{
#else
	if (nvram_match("debug_cprintf", "1")) {
#endif
		if ((f = fopen("/dev/console", "w")) != NULL) {
			va_start(args, format);
			vfprintf(f, format, args);
			va_end(args);
			fclose(f);
		}
	}
#if 1	
	if (nvram_match("debug_cprintf_file", "1")) {
//		char s[32];
//		sprintf(s, "/tmp/cprintf.%d", getpid());
//		if ((f = fopen(s, "a")) != NULL) {
		if ((f = fopen("/tmp/cprintf", "a")) != NULL) {
			va_start(args, format);
			vfprintf(f, format, args);
			va_end(args);
			fclose(f);
		}
	}
#endif
#if 0
	if (nvram_match("debug_cprintf_log", "1")) {
		char s[512];
		
		va_start(args, format);
		vsnprintf(s, sizeof(s), format, args);
		s[sizeof(s) - 1] = 0;
		va_end(args);
		
		if ((s[0] != '\n') || (s[1] != 0)) syslog(LOG_DEBUG, "%s", s);
	}
#endif
}


#ifndef WL_BSS_INFO_VERSION
#error WL_BSS_INFO_VERSION
#endif

#if WL_BSS_INFO_VERSION >= 108
// xref (all): nas, wlconf

#ifndef MAX_NVPARSE
#define MAX_NVPARSE 255
#endif

#if 0
/*
 * Get the ip configuration index if it exists given the
 * eth name.
 *
 * @param	wl_ifname 	pointer to eth interface name
 * @return	index or -1 if not found
 */
int
get_ipconfig_index(char *eth_ifname)
{
	char varname[64];
	char varval[64];
	char *ptr;
	char wl_ifname[NVRAM_MAX_PARAM_LEN];
	int index;

	/* Bail if we get a NULL or empty string */

	if (!eth_ifname) return -1;
	if (!*eth_ifname) return -1;

	/* Look up wl name from the eth name */
	if (osifname_to_nvifname(eth_ifname, wl_ifname, sizeof(wl_ifname)) != 0)
		return -1;

	snprintf(varname, sizeof(varname), "%s_ipconfig_index", wl_ifname);

	ptr = nvram_get(varname);

	if (ptr) {
	/* Check ipconfig_index pointer to see if it is still pointing
	 * the correct lan config block
	 */
		if (*ptr) {
			int index;
			char *ifname;
			char buf[64];
			index = atoi(ptr);

			snprintf(buf, sizeof(buf), "lan%d_ifname", index);

			ifname = nvram_get(buf);

			if (ifname) {
				if  (!(strcmp(ifname, wl_ifname)))
					return index;
			}
			nvram_unset(varname);
		}
	}

	/* The index pointer may not have been configured if the
	 * user enters the variables manually. Do a brute force search
	 *  of the lanXX_ifname variables
	 */
	for (index = 0; index < MAX_NVPARSE; index++) {
		snprintf(varname, sizeof(varname), "lan%d_ifname", index);
		if (nvram_match(varname, wl_ifname)) {
			/* if a match is found set up a corresponding index pointer for wlXX */
			snprintf(varname, sizeof(varname), "%s_ipconfig_index", wl_ifname);
			snprintf(varval, sizeof(varval), "%d", index);
			nvram_set(varname, varval);
			nvram_commit();
			return index;
		};
	}
	return -1;
}

/*
 * Set the ip configuration index given the eth name
 * Updates both wlXX_ipconfig_index and lanYY_ifname.
 *
 * @param	eth_ifname 	pointer to eth interface name
 * @return	0 if successful -1 if not.
 */
int
set_ipconfig_index(char *eth_ifname, int index)
{
	char varname[255];
	char varval[16];
	char wl_ifname[NVRAM_MAX_PARAM_LEN];

	/* Bail if we get a NULL or empty string */

	if (!eth_ifname) return -1;
	if (!*eth_ifname) return -1;

	if (index >= MAX_NVPARSE) return -1;

	/* Look up wl name from the eth name only if the name contains
	   eth
	*/

	if (osifname_to_nvifname(eth_ifname, wl_ifname, sizeof(wl_ifname)) != 0)
		return -1;

	snprintf(varname, sizeof(varname), "%s_ipconfig_index", wl_ifname);
	snprintf(varval, sizeof(varval), "%d", index);
	nvram_set(varname, varval);

	snprintf(varname, sizeof(varname), "lan%d_ifname", index);
	nvram_set(varname, wl_ifname);

	nvram_commit();

	return 0;
}

/*
 * Get interfaces belonging to a specific bridge.
 *
 * @param	bridge_name 	pointer to bridge interface name
 * @return	list of interfaces belonging to the bridge or NULL
 *              if not found/empty
 */
char *
get_bridged_interfaces(char *bridge_name)
{
	static char interfaces[255];
	char *ifnames = NULL;
	char bridge[64];

	if (!bridge_name) return NULL;

	memset(interfaces, 0, sizeof(interfaces));
	snprintf(bridge, sizeof(bridge), "%s_ifnames", bridge_name);

	ifnames = nvram_get(bridge);

	if (ifnames)
		strncpy(interfaces, ifnames, sizeof(interfaces));
	else
		return NULL;

	return  interfaces;

}

#endif	// 0

/*
 * Search a string backwards for a set of characters
 * This is the reverse version of strspn()
 *
 * @param	s	string to search backwards
 * @param	accept	set of chars for which to search
 * @return	number of characters in the trailing segment of s
 *		which consist only of characters from accept.
 */
static size_t
sh_strrspn(const char *s, const char *accept)
{
	const char *p;
	size_t accept_len = strlen(accept);
	int i;


	if (s[0] == '\0')
		return 0;

	p = s + strlen(s);
	i = 0;

	do {
		p--;
		if (memchr(accept, *p, accept_len) == NULL)
			break;
		i++;
	} while (p != s);

	return i;
}

/*
 * Parse the unit and subunit from an interface string such as wlXX or wlXX.YY
 *
 * @param	ifname	interface string to parse
 * @param	unit	pointer to return the unit number, may pass NULL
 * @param	subunit	pointer to return the subunit number, may pass NULL
 * @return	Returns 0 if the string ends with digits or digits.digits, -1 otherwise.
 *		If ifname ends in digits.digits, then unit and subuint are set
 *		to the first and second values respectively. If ifname ends
 *		in just digits, unit is set to the value, and subunit is set
 *		to -1. On error both unit and subunit are -1. NULL may be passed
 *		for unit and/or subuint to ignore the value.
 */
int
get_ifname_unit(const char* ifname, int *unit, int *subunit)
{
	const char digits[] = "0123456789";
	char str[64];
	char *p;
	size_t ifname_len = strlen(ifname);
	size_t len;
	unsigned long val;

	if (unit)
		*unit = -1;
	if (subunit)
		*subunit = -1;

	if (ifname_len + 1 > sizeof(str))
		return -1;

	strcpy(str, ifname);

	/* find the trailing digit chars */
	len = sh_strrspn(str, digits);

	/* fail if there were no trailing digits */
	if (len == 0)
		return -1;

	/* point to the beginning of the last integer and convert */
	p = str + (ifname_len - len);
	val = strtoul(p, NULL, 10);

	/* if we are at the beginning of the string, or the previous
	 * character is not a '.', then we have the unit number and
	 * we are done parsing
	 */
	if (p == str || p[-1] != '.') {
		if (unit)
			*unit = val;
		return 0;
	} else {
		if (subunit)
			*subunit = val;
	}

	/* chop off the '.NNN' and get the unit number */
	p--;
	p[0] = '\0';

	/* find the trailing digit chars */
	len = sh_strrspn(str, digits);

	/* fail if there were no trailing digits */
	if (len == 0)
		return -1;

	/* point to the beginning of the last integer and convert */
	p = p - len;
	val = strtoul(p, NULL, 10);

	/* save the unit number */
	if (unit)
		*unit = val;

	return 0;
}

/* In the space-separated/null-terminated list(haystack), try to
 * locate the string "needle"
 */
char *find_in_list(const char *haystack, const char *needle)
{
	const char *ptr = haystack;
	int needle_len = 0;
	int haystack_len = 0;
	int len = 0;

	if (!haystack || !needle || !*haystack || !*needle)
		return NULL;

	needle_len = strlen(needle);
	haystack_len = strlen(haystack);

	while (*ptr != 0 && ptr < &haystack[haystack_len])
	{
		/* consume leading spaces */
		ptr += strspn(ptr, " ");

		/* what's the length of the next word */
		len = strcspn(ptr, " ");

		if ((needle_len == len) && (!strncmp(needle, ptr, len)))
			return (char*) ptr;

		ptr += len;
	}
	return NULL;
}


/**
 *	remove_from_list
 *	Remove the specified word from the list.

 *	@param name word to be removed from the list
 *	@param list Space separated list to modify
 *	@param listsize Max size the list can occupy

 *	@return	error code
 */
int remove_from_list(const char *name, char *list, int listsize)
{
	int listlen = 0;
	int namelen = 0;
	char *occurrence = list;

	if (!list || !name || (listsize <= 0))
		return EINVAL;

	listlen = strlen(list);
	namelen = strlen(name);

	occurrence = find_in_list(occurrence, name);

	if (!occurrence)
		return EINVAL;

	/* last item in list? */
	if (occurrence[namelen] == 0)
	{
		/* only item in list? */
		if (occurrence != list)
			occurrence--;
		occurrence[0] = 0;
	}
	else if (occurrence[namelen] == ' ')
	{
		strncpy(occurrence, &occurrence[namelen+1 /* space */],
		        strlen(&occurrence[namelen+1 /* space */]) +1 /* terminate */);
	}

	return 0;
}

/**
 *		add_to_list
 *	Add the specified interface(string) to the list as long as
 *	it will fit in the space left in the list.

 *	NOTE: If item is already in list, it won't be added again.

 *	@param name Name of interface to be added to the list
 *	@param list List to modify
 *	@param listsize Max size the list can occupy

 *	@return	error code
 */
int add_to_list(const char *name, char *list, int listsize)
{
	int listlen = 0;
	int namelen = 0;

	if (!list || !name || (listsize <= 0))
		return EINVAL;

	listlen = strlen(list);
	namelen = strlen(name);

	/* is the item already in the list? */
	if (find_in_list(list, name))
		return 0;

	if (listsize <= listlen + namelen + 1 /* space */ + 1 /* NULL */)
		return EMSGSIZE;

	/* add a space if the list isn't empty and it doesn't already have space */
	if (list[0] != 0 && list[listlen-1] != ' ')
	{
		list[listlen++] = 0x20;
	}

	strncpy(&list[listlen], name, namelen + 1 /* terminate */);

	return 0;
}

/* Utility function to remove duplicate entries in a space separated list */

char *remove_dups(char *inlist, int inlist_size)
{
	char name[256], *next = NULL;
	char *outlist;

	if (!inlist_size) return NULL;
	if (!inlist) return NULL;

	outlist = (char *) malloc(inlist_size);
	if (!outlist) return NULL;
	memset(outlist, 0, inlist_size);

	foreach(name, inlist, next) {
		if (!find_in_list(outlist, name)) {
			if (strlen(outlist) == 0) {
				snprintf(outlist, inlist_size, "%s", name);
			} else {
				strncat(outlist, " ",  inlist_size - strlen(outlist));
				strncat(outlist, name, inlist_size - strlen(outlist));
			}
		}
	}
	strncpy(inlist, outlist, inlist_size);

	free(outlist);
	return inlist;
}

char *find_smallest_in_list(char *haystack) {
	char *ptr = haystack;
	char *smallest = ptr;
	int haystack_len = strlen(haystack);
	int len = 0;

	if (!haystack || !*haystack || !haystack_len)
		return NULL;

	while (*ptr != 0 && ptr < &haystack[haystack_len]) {
		/* consume leading spaces */
		ptr += strspn(ptr, " ");

		/* what's the length of the next word */
		len = strcspn(ptr, " ");

		/* if this item/word is 'smaller', update our pointer */
		if ((strncmp(smallest, ptr, len) > 0)) {
			smallest = ptr;
		}

		ptr += len;
	}
	return (char*) smallest;
}

char *sort_list(char *inlist, int inlist_size) {
	char *tmplist;
	char tmp[IFNAMSIZ];

	if (!inlist_size) return NULL;
	if (!inlist) return NULL;

	tmplist = (char *) malloc(inlist_size);
	if (!tmplist) return NULL;
	memset(tmplist, 0, inlist_size);

	char *b;
	int len;
	while ((b = find_smallest_in_list(inlist)) != NULL) {
		len = strcspn(b, " ");
		snprintf(tmp, len + 1, "%s", b);

		add_to_list(tmp, tmplist, inlist_size);
		remove_from_list(tmp, inlist, inlist_size);

	}
	strncpy(inlist, tmplist, inlist_size);

	free(tmplist);
	return inlist;
}

/*
	 return true/false if any wireless interface has URE enabled.
*/
int
ure_any_enabled(void)
{
	char *temp;
	char nv_param[NVRAM_MAX_PARAM_LEN];

	sprintf(nv_param, "ure_disable");
	temp = nvram_safe_get(nv_param);
	if (temp && (strncmp(temp, "0", 1) == 0))
		return 1;
	else
		return 0;
}

#define WLMBSS_DEV_NAME	"wlmbss"
#define WL_DEV_NAME "wl"
#define WDS_DEV_NAME	"wds"

/**
 *	 nvifname_to_osifname()
 *  The intent here is to provide a conversion between the OS interface name
 *  and the device name that we keep in NVRAM.
 * This should eventually be placed in a Linux specific file with other
 * OS abstraction functions.

 * @param nvifname pointer to ifname to be converted
 * @param osifname_buf storage for the converted osifname
 * @param osifname_buf_len length of storage for osifname_buf
 */
int
nvifname_to_osifname(const char *nvifname, char *osifname_buf,
                     int osifname_buf_len)
{
	char varname[NVRAM_MAX_PARAM_LEN];
	char *ptr;

	memset(osifname_buf, 0, osifname_buf_len);

	/* Bail if we get a NULL or empty string */
	if ((!nvifname) || (!*nvifname) || (!osifname_buf)) {
		return -1;
	}

	if (strstr(nvifname, "eth") || strstr(nvifname, ".")) {
		strncpy(osifname_buf, nvifname, osifname_buf_len);
		return 0;
	}

	snprintf(varname, sizeof(varname), "%s_ifname", nvifname);
	ptr = nvram_get(varname);
	if (ptr) {
		/* Bail if the string is empty */
		if (!*ptr) return -1;
		strncpy(osifname_buf, ptr, osifname_buf_len);
		return 0;
	}

	return -1;
}

/* osifname_to_nvifname()
 * Convert the OS interface name to the name we use internally(NVRAM, GUI, etc.)
 * This is the Linux version of this function

 * @param osifname pointer to osifname to be converted
 * @param nvifname_buf storage for the converted ifname
 * @param nvifname_buf_len length of storage for nvifname_buf
 */
int
osifname_to_nvifname(const char *osifname, char *nvifname_buf,
                     int nvifname_buf_len)
{
	char varname[NVRAM_MAX_PARAM_LEN];
	int pri, sec;

	/* Bail if we get a NULL or empty string */

	if ((!osifname) || (!*osifname) || (!nvifname_buf))
	{
		return -1;
	}

	memset(nvifname_buf, 0, nvifname_buf_len);

	if (strstr(osifname, "wl") || strstr(osifname, "br") ||
	     strstr(osifname, "wds")) {
		strncpy(nvifname_buf, osifname, nvifname_buf_len);
		return 0;
	}

	/* look for interface name on the primary interfaces first */
	for (pri = 0; pri < MAX_NVPARSE; pri++) {
		snprintf(varname, sizeof(varname),
					"wl%d_ifname", pri);
		if (nvram_match(varname, (char *)osifname)) {
					snprintf(nvifname_buf, nvifname_buf_len, "wl%d", pri);
					return 0;
				}
	}

	/* look for interface name on the multi-instance interfaces */
	for (pri = 0; pri < MAX_NVPARSE; pri++)
		for (sec = 0; sec < MAX_NVPARSE; sec++) {
			snprintf(varname, sizeof(varname),
					"wl%d.%d_ifname", pri, sec);
			if (nvram_match(varname, (char *)osifname)) {
				snprintf(nvifname_buf, nvifname_buf_len, "wl%d.%d", pri, sec);
				return 0;
			}
		}

	return -1;
}

#endif	// #if WL_BSS_INFO_VERSION >= 108





// -----------------------------------------------------------------------------




#if 0
/*
 * Reads file and returns contents
 * @param	fd	file descriptor
 * @return	contents of file or NULL if an error occurred
 */
char *fd2str(int fd)
{
	char *buf = NULL;
	size_t count = 0, n;

	do {
		buf = realloc(buf, count + 512);
		n = read(fd, buf + count, 512);
		if (n < 0) {
			free(buf);
			buf = NULL;
		}
		count += n;
	} while (n == 512);

	close(fd);
	if (buf)
		buf[count] = '\0';
	return buf;
}

/*
 * Reads file and returns contents
 * @param	path	path to file
 * @return	contents of file or NULL if an error occurred
 */
char *file2str(const char *path)
{
	int fd;

	if ((fd = open(path, O_RDONLY)) == -1) {
		perror(path);
		return NULL;
	}

	return fd2str(fd);
}

/*
 * Waits for a file descriptor to change status or unblocked signal
 * @param	fd	file descriptor
 * @param	timeout	seconds to wait before timing out or 0 for no timeout
 * @return	1 if descriptor changed status or 0 if timed out or -1 on error
 */

int waitfor(int fd, int timeout)
{
	fd_set rfds;
	struct timeval tv = { timeout, 0 };

	FD_ZERO(&rfds);
	FD_SET(fd, &rfds);
	return select(fd + 1, &rfds, NULL, NULL, (timeout > 0) ? &tv : NULL);
}

/*
 * Kills process whose PID is stored in plaintext in pidfile
 * @param	pidfile	PID file
 * @return	0 on success and errno on failure
 */
int kill_pidfile(char *pidfile)
{
	FILE *fp;
	char buf[256];

	if ((fp = fopen(pidfile, "r")) != NULL) {
		if (fgets(buf, sizeof(buf), fp)) {
			pid_t pid = strtoul(buf, NULL, 0);
			fclose(fp);
			return kill(pid, SIGTERM);
		}
		fclose(fp);
  	}
	return errno;
}
#endif	// 0
