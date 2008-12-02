#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h> /* for strerror() */
#include <errno.h>
#include "busybox.h"


extern int optind; /* in unistd.h */
extern int errno;  /* for use with strerror() */
static char *opt_pattern = NULL;

extern int pgrep_main(int argc, char **argv)
{
	int opt;
	pid_t* pidList;

	/* do normal option parsing */
	while ((opt = getopt(argc, argv, "f")) > 0) {
		switch (opt) {
			case 'f':
				break;
			default:
				show_usage();
		}
	}

	if (argc - optind == 1)
                opt_pattern = argv[optind];

	if (!opt_pattern)
                show_usage();

	pidList = find_pid_by_name(opt_pattern);

	if (!pidList || *pidList<=0) {
		return EXIT_SUCCESS;
	}

	for(; pidList && *pidList!=0; pidList++) {
		printf("%d\n", *pidList);	
	}

	return EXIT_SUCCESS;
}
