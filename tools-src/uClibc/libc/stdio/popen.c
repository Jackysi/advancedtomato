/*
 * Modified     3/03/2001       Manuel Novoa III
 *
 * Added check for legal mode arg.
 * Call fdopen and check return value before forking.
 * Reduced code size by using variables pr and pnr instead of array refs.
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

/* uClinux-2.0 has vfork, but Linux 2.0 doesn't */
#include <sys/syscall.h>
#if ! defined __NR_vfork
#define vfork fork	
#endif

FILE *popen (const char *command, const char *mode)
{
	FILE *fp;
	int pipe_fd[2];
	int pid, reading;
	int pr, pnr;

	reading = (mode[0] == 'r');
	if ((!reading && (mode[0] != 'w')) || mode[1]) {
		__set_errno(EINVAL);			/* Invalid mode arg. */
	} else if (pipe(pipe_fd) == 0) {
		pr = pipe_fd[reading];
		pnr = pipe_fd[1-reading];
		if ((fp = fdopen(pnr, mode)) != NULL) {
			if ((pid = vfork()) == 0) {	/* vfork -- child */
				close(pnr);
				if (pr != reading) {
					close(reading);
					dup2(pr, reading);
					close(pr);
				}
				execl("/bin/sh", "sh", "-c", command, (char *) 0);
				_exit(255);		/* execl failed! */
			} else {			/* vfork -- parent or failed */
				close(pr);
				if (pid > 0) {	/* vfork -- parent */
					return fp;
				} else {		/* vfork -- failed! */
					fclose(fp);
				}
			}
		} else {				/* fdopen failed */
			close(pr);
			close(pnr);
		}
	}
	return NULL;
}

int pclose(FILE *fd)
{
	int waitstat;

	if (fclose(fd) != 0) {
		return EOF;
	}
	if (wait(&waitstat) == -1)
		return -1;
	return waitstat;
}



