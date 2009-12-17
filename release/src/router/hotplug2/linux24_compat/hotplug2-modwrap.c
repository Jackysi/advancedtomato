/*****************************************************************************\
*  _  _       _          _              ___                                   *
* | || | ___ | |_  _ __ | | _  _  __ _ |_  )                                  *
* | __ |/ _ \|  _|| '_ \| || || |/ _` | / /                                   *
* |_||_|\___/ \__|| .__/|_| \_,_|\__, |/___|                                  *
*                 |_|            |___/                                        *
\*****************************************************************************/

#define _GNU_SOURCE

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <ctype.h>
#include <fnmatch.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/utsname.h>
#include <linux/types.h>
#include <linux/input.h>

#include "../hotplug2.h"
#include "../mem_utils.h"
#include "../parser_utils.h"
#include "../filemap_utils.h"

int execute(char **argv) {
	pid_t p;
	
	p = fork();
	switch (p) {
		case -1:
			return -1;
		case 0:
			execvp(argv[0], argv);
			exit(1);
			break;
		default:
			waitpid(p, NULL, 0);
			break;
	}
	return 0;
}

int main(int argc, char *argv[]) {
	struct utsname unamebuf;
	struct filemap_t aliasmap;
	char *line, *nline, *nptr;
	char *token;
	char *filename;
	
	char *cur_alias, *match_alias, *module;
	
	if (argc < 2) {
		fprintf(stderr, "Usage: hotplug2-modwrap [options for modprobe] <alias>\n");
	}
	
	match_alias = strdup(argv[argc - 1]);
	
	if (uname(&unamebuf)) {
		ERROR("uname", "Unable to perform uname: %s.", strerror(errno));
		return 1;
	}
	
	/* We use this one */
	argv[0] = getenv("MODPROBE_COMMAND");
	if (argv[0] == NULL)
		argv[0] = "/sbin/modprobe";
	
	/* "/lib/modules/" + "/" + "\0" */
	filename = xmalloc(15 + strlen(unamebuf.release) + strlen("modules.alias"));
	strcpy(filename, "/lib/modules/");
	strcat(filename, unamebuf.release);
	strcat(filename, "/modules.alias");
	
	if (map_file(filename, &aliasmap)) {
		ERROR("map_file", "Unable to map file: `%s'.", filename);
		free(filename);
		free(match_alias);
		return 1;
	}
	
	nptr = aliasmap.map;
	while ((line = dup_line(nptr, &nptr)) != NULL) {
		nline = line;
		
		token = dup_token(nline, &nline, isspace);
		if (!token || strcmp(token, "alias")) {
			free(token);
			free(line);
			continue;
		}
		free(token);
		
		cur_alias = dup_token(nline, &nline, isspace);
		if (!cur_alias) {
			free(line);
			continue;
		}
		
		module = dup_token(nline, &nline, isspace);
		if (!module) {
			free(line);
			free(cur_alias);
			continue;
		}
		
		if (!fnmatch(cur_alias, match_alias, 0)) {
			argv[argc - 1] = module;
			if (execute(argv)) {
				ERROR("execute", "Unable to execute: `%s'.", argv[0]);
			}
		}
		
		free(cur_alias);
		free(module);
		free(line);
	}
	
	free(filename);
	free(match_alias);
	unmap_file(&aliasmap);
	
	return 0;
}
