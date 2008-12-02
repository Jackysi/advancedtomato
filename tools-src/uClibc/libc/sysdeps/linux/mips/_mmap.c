/* Use new style mmap for mips */
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/syscall.h>

#if 0
#ifdef __NR_mmap2
# undef __NR_mmap
# define __NR_mmap __NR_mmap2
#endif
#endif

_syscall6 (__ptr_t, mmap, __ptr_t, addr, size_t, len, int, prot,
	   int, flags, int, fd, __off_t, offset);
