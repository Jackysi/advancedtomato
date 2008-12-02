/*
 *   ktab implementation for busybox by Broadcom Corporation
 *   Based on ktab-0.4.0
 *   Directives and variables unsupported, minimal chatter
 *
 *   KTAB - Keith program TABing, src/ktab.c
 *   Copyright (C) 1998 Keith Fralick
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 1, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>
#include <signal.h>
#include <unistd.h>
#include "busybox.h"

/*
 * config.h
 */

/*
 * MIN_PROGRAM_CHECK
 *  
 * Define this to the minimum time in which a script may contain
 * for checking a program's existance.  This number is in seconds.
 */
#define	MIN_PROGRAM_CHECK	30

/*
 * DEFAULT_TIME
 *
 * Define this value to the value in which shall be default if a
 * user supplies the -p option for a KTAB.  This number is in
 * minutes.
 */
#define	DEFAULT_TIME		3	

/*
 * READ_WRITE_PERMISSION
 *
 * Define this if you want your users to have read *AND* write
 * permissions in order to ktab a process under their UID.
 */
#define READ_WRITE_PERMISSION

/*
 * CHECK_RUN_FROM_FEET
 *
 * Define this to the value that you want KTAB to wait until
 * the period that it is off its feet in order to check the
 * desired process for the very first time.  This number is
 * in seconds.
 */
#define CHECK_RUN_FROM_FEET	10	

/*
 * SUPRESS_PROGRAM_DATA
 *
 * Define this if you want to attempt to supress any data that
 * the programs you run might try to print to the screen.  However,
 * this is only an attempt, if you undef this, you may still use
 * the '-' character in front of the path to your programs in
 * the script.  However, defining this, it will attempt to supress
 * whether or not you have the '-' character.
 */
#undef SUPRESS_PROGRAM_DATA 

/*
 * version.h
 */

#define KTAB_VERSION	"0.4.0"

/*
 * struct.h
 */

typedef struct processes Proc;
typedef struct shortcuts Short;

struct	processes	{
	char	path[1024];
	char	args[1024];		/* this is for arguments v0.3.1 */
	time_t	chktime;
	time_t	lastchk;
	struct	processes *next;
} *proc = NULL;

struct	shortcuts	{
	char	name[256];
	char	real[1024];
	int	num;
	struct	shortcuts *next;
} *shorty = NULL;

/*
 * ktab.c
 */

extern	void	add_process();
extern	void	do_script();
extern	void	check_jobs();
extern	void	rehash();
extern	void	loop_procs();
extern	void	file_exists(char *);

extern	Short	*make_short();

FILE	*i, *log, *ma;

#define	mycmp	strcasecmp

#define O_RW	 "rw"
#define O_APPEND  "a"
#ifndef READ_WRITE_PERMISSION
#define	O_RDONLY "r"
#else
#define O_RDONLY O_RW
#endif

int	dupp = DEFAULT_TIME * 60;
char	*logfile = NULL;
char	*prog = NULL, *sfile = NULL;
char	*curpath, *os, *shell, *home, *myhost, *trm, *luser;
char	tmp[1024];

int	opt_kill = 0;

int	review = 0, cntt = 0;

static	pid_t	w;

int ktab_main(int argc, char *argv[])
{
	time_t a = time(NULL) + CHECK_RUN_FROM_FEET;
	int opt;

	curpath = getenv("PWD");

	while ((opt = getopt(argc, argv, "f:p:r:")) != -1) {
		switch (opt) {
		case 'f':
			sfile = strdup(optarg);
			break;
		case 'p':
			prog = strdup(optarg);
			break;
		case 'r':
			sfile = strdup(optarg);
			review = 1;
			break;
		case 'v':
			printf("KTAB\tBy: Keith Fralick\tv%s\n\n", KTAB_VERSION);
			/* fall through */
		default:
			show_usage();
		}
	}

	if(!((i = fopen(sfile, O_RDONLY)) ||
		(i = fopen(prog, O_RDONLY))))
		perror_msg_and_die(sfile ? sfile : prog);
	
	if (prog)
		fclose(i);
	else
		do_script();
	if(review)
	{
		review = 0;
		perror_msg_and_die("%d errors\n", cntt);
	}
	w = getpid() + 1;
	argv[1] = argv[2] = NULL;
	if (daemon(1,0) < 0)
		exit(0);
	(void)signal(SIGHUP, rehash);
	if(prog)
		while (1)
		{
			if(curpath) chdir(curpath);
			while(time(NULL) <= a) sleep(1);
			if(prog)
				system(prog);
			a+=dupp;
		}
	if(sfile)
		while (1)
		{
			if(curpath) chdir(curpath);
			check_jobs();
			sleep(1);			
		}
	rehash(0);
	return 0;
}

void	add_process(int t, char pathto[1024], char args[1024])
{
	Proc 	*new, *current;
	char	tog[1024];	

	if(!(new = (Proc *)malloc(sizeof(Proc))))
		perror_msg_and_die("malloc");
	if(new != NULL)
		new->next = NULL;
	if(args)
		(void)sprintf(pathto, "%s %s", pathto, args);
	else
		strcpy(tog, pathto);
	strcpy(new->path, pathto);
	new->chktime = t;
	new->lastchk = 0;
	if(proc == NULL) 
		proc = new;
	else
	{
		current = proc;
		while(current->next)
			current = current->next;
		current->next = new;
	}
}

void	do_script()
{
	int	c = 0, p = 0;
	int	mvalue = 0;
	char	*f, *s;

	while(fgets(tmp, sizeof(tmp), i))
	{
		if(!tmp || *tmp == ' ' || !strchr(tmp, ' ') ||
			*tmp == '#')
			continue; 
		if(strlen(tmp) > 2048)
			continue;
		while(strchr(tmp, '\n'))
			*strchr(tmp, '\n') = '\0';
		if(*tmp == 's')
			mvalue = 1;
		else if(*tmp == 'm')
			mvalue = 60;
		else if(*tmp == 'h')
			mvalue = 3600;
		else if(*tmp == 'd')
			mvalue = 3600 * 24;
		f = tmp;
		s = strchr(tmp, ' ');
		*(s)++ = '\0';
		if(mvalue)
			f++;
		else
			mvalue = 60;
		p = atoi(f);
		p*=mvalue; 
		if (p == 0)
			p = mvalue ? mvalue : (DEFAULT_TIME * 60);
		if(p < MIN_PROGRAM_CHECK)
			perror_msg_and_die("minimum time %d s", MIN_PROGRAM_CHECK);
		if(mvalue == 60 && p < CHECK_RUN_FROM_FEET)
			p = MIN_PROGRAM_CHECK * 60;
		if(strchr(s, ' '))
		{
			f = s;
			s = strchr(f, ' ');
			*(s)++ = '\0';
		}
		else
			f = s;
		file_exists(f);
		add_process(p, f, s ? s : NULL);		
		c++;
	}
	if(!c)
	 exit (-1);
	fclose(i);
}

void	file_exists(char *filename)
{
	FILE	*a;

	if(*filename == '-')
		filename++;
	if(!((a = fopen(filename, O_RDONLY))))
		perror_msg_and_die(filename);
	fclose(a);
}

void	check_jobs()
{
	Proc	*z;
	char	*kind;

	for(z = proc; z; z = z->next)
	{
		if(z->lastchk == 0)
			z->lastchk = time(NULL) + CHECK_RUN_FROM_FEET;
		else
			if(time(NULL) - z->lastchk >= z->chktime)
			{			
				strcpy(tmp, z->path);
#ifdef 	SUPRESS_PROGRAM_DATA
				kind = tmp;
				if(*kind == '-') kind++;
				strcpy(tmp, kind);
				strcat(tmp, ">> /tmp/blah");
# else
				if(*tmp == '-')	
					{
						kind = tmp;
						kind++;
						strcpy(tmp, kind);
						strcat(tmp, ">> /tmp/blah");
					}
#endif
				system(tmp);
				z->lastchk = time(NULL);
			} 
	}
}

/*
 * When we get signal SIGHUP, we just die.
 * The reason being, why flush out the 
 * structure and start again?  If they want
 * to reload something, do it right.
 */
void	rehash(x)
{
	if(prog)
		free(prog);
	if(sfile)
		free(sfile);
	exit(0);
}

Short	*make_short()
{
        Short *new, *current;
        if(!(new = (Short *)malloc(sizeof(Short))))
		perror_msg_and_die("malloc");
        if(new != NULL)
                new->next = NULL;
        if(shorty == NULL)
                shorty = new;
        else
        {
                current = shorty;
                while(current->next)
                        current = current->next;
                current->next = new;
        }
	return (new);
}
