/*
 * mdstat - parse /proc/mdstat file. Part of:
 * mdadm - manage Linux "md" devices aka RAID arrays.
 *
 * Copyright (C) 2002-2006 Neil Brown <neilb@suse.de>
 *
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *    Author: Neil Brown
 *    Email: <neilb@cse.unsw.edu.au>
 *    Paper: Neil Brown
 *           School of Computer Science and Engineering
 *           The University of New South Wales
 *           Sydney, 2052
 *           Australia
 */

/*
 * The /proc/mdstat file comes in at least 3 flavours:
 * In an unpatched 2.2 kernel (md 0.36.6):
 *  Personalities : [n raidx] ...
 *  read_ahead {not set|%d sectors}
 *  md0 : {in}active{ raidX /dev/hda...  %d blocks{ maxfault=%d}}
 *  md1 : .....
 *
 * Normally only 4 md lines, but all are listed.
 *
 * In a patched 2.2 kernel (md 0.90.0)
 *  Personalities : [raidx] ...
 *  read_ahead {not set|%d sectors}
 *  mdN : {in}active {(readonly)} raidX dev[%d]{(F)} ... %d blocks STATUS RESYNC
 *  ... Only initialised arrays listed
 *  unused devices: {dev dev ... | <none>}
 *
 * STATUS is personality dependant:
 *    linear:  %dk rounding
 *    raid0:   %dk chunks
 *    raid1:   [%d/%d] [U_U]   ( raid/working.  operational or not)
 *    raid5:   level 4/5, %dk chunk, algorithm %d [%d/%d] [U_U]
 *
 * RESYNC is  empty or:
 *    {resync|recovery}=%u%% finish=%u.%umin
 *  or
 *    resync=DELAYED
 *
 * In a 2.4 kernel (md 0.90.0/2.4)
 *  Personalities : [raidX] ...
 *  read_ahead {not set|%d sectors}
 *  mdN : {in}active {(read-only)} raidX dev[%d]{(F)} ...
 *       %d blocks STATUS
 *       RESYNC
 *  unused devices: {dev dev .. | <none>}
 *
 *  STATUS matches 0.90.0/2.2
 *  RESYNC includes [===>....],
 *          adds a space after {resync|recovery} and before and after '='
 *          adds a decimal to the recovery percent.
 *	    adds (%d/%d) resync amount  and max_blocks, before finish.
 *	    adds speed=%dK/sec after finish
 *
 *
 *
 * Out of this we want to extract:
 *   list of devices, active or not
 *   pattern of failed drives (so need number of drives)
 *   percent resync complete
 *
 * As continuation is indicated by leading space, we use
 *  conf_line from config.c to read logical lines
 *
 */

#include	"mdadm.h"
#include	"dlink.h"
#include	<sys/select.h>

void free_mdstat(struct mdstat_ent *ms)
{
	while (ms) {
		struct mdstat_ent *t;
		if (ms->dev) free(ms->dev);
		if (ms->level) free(ms->level);
		if (ms->pattern) free(ms->pattern);
		t = ms;
		ms = ms->next;
		free(t);
	}
}

static int mdstat_fd = -1;
struct mdstat_ent *mdstat_read(int hold, int start)
{
	FILE *f;
	struct mdstat_ent *all, *rv, **end, **insert_here;
	char *line;

	if (hold && mdstat_fd != -1) {
		lseek(mdstat_fd, 0L, 0);
		f = fdopen(dup(mdstat_fd), "r");
	} else
		f = fopen("/proc/mdstat", "r");
	if (f == NULL)
		return NULL;

	all = NULL;
	end = &all;
	for (; (line = conf_line(f)) ; free_line(line)) {
		struct mdstat_ent *ent;
		char *w;
		int devnum;
		int in_devs = 0;
		char *ep;

		if (strcmp(line, "Personalities")==0)
			continue;
		if (strcmp(line, "read_ahead")==0)
			continue;
		if (strcmp(line, "unused")==0)
			continue;
		insert_here = NULL;
		/* Better be an md line.. */
		if (strncmp(line, "md", 2)!= 0)
			continue;
		if (strncmp(line, "md_d", 4) == 0)
			devnum = -1-strtoul(line+4, &ep, 10);
		else if (strncmp(line, "md", 2) == 0)
			devnum = strtoul(line+2, &ep, 10);
		else
			continue;
		if (ep == NULL || *ep ) {
			/* fprintf(stderr, Name ": bad /proc/mdstat line starts: %s\n", line); */
			continue;
		}

		ent = malloc(sizeof(*ent));
		if (!ent) {
			fprintf(stderr, Name ": malloc failed reading /proc/mdstat.\n");
			free_line(line);
			break;
		}
		ent->dev = ent->level = ent->pattern= NULL;
		ent->next = NULL;
		ent->percent = -1;
		ent->active = -1;
		ent->resync = 0;

		ent->dev = strdup(line);
		ent->devnum = devnum;
		
		for (w=dl_next(line); w!= line ; w=dl_next(w)) {
			int l = strlen(w);
			char *eq;
			if (strcmp(w, "active")==0)
				ent->active = 1;
			else if (strcmp(w, "inactive")==0)
				ent->active = 0;
			else if (ent->active >=0 &&
				 ent->level == NULL &&
				 w[0] != '(' /*readonly*/) {
				ent->level = strdup(w);
				in_devs = 1;
			} else if (in_devs && strcmp(w, "blocks")==0)
				in_devs = 0;
			else if (in_devs && strncmp(w, "md", 2)==0) {
				/* This has an md device as a component.
				 * If that device is already in the list,
				 * make sure we insert before there.
				 */
				struct mdstat_ent **ih;
				int dn2;
				if (strncmp(w, "md_d", 4)==0)
					dn2 = -1-strtoul(w+4, &ep, 10);
				else
					dn2 = strtoul(w+2, &ep, 10);
				ih = &all;
				while (ih != insert_here && *ih &&
				       (*ih)->devnum != dn2)
					ih = & (*ih)->next;
				insert_here = ih;
			} else if (!ent->pattern &&
				 w[0] == '[' &&
				 (w[1] == 'U' || w[1] == '_')) {
				ent->pattern = strdup(w+1);
				if (ent->pattern[l-2]==']')
					ent->pattern[l-2] = '\0';
			} else if (ent->percent == -1 &&
				   strncmp(w, "re", 2)== 0 &&
				   w[l-1] == '%' &&
				   (eq=strchr(w, '=')) != NULL ) {
				ent->percent = atoi(eq+1);
				if (strncmp(w,"resync", 4)==0)
					ent->resync = 1;
			} else if (ent->percent == -1 &&
				   strncmp(w, "resync", 4)==0) {
				ent->resync = 1;
			} else if (ent->percent == -1 &&
				   w[0] >= '0' && 
				   w[0] <= '9' &&
				   w[l-1] == '%') {
				ent->percent = atoi(w);
			}
		}
		if (insert_here && (*insert_here)) {
			ent->next = *insert_here;
			*insert_here = ent;
		} else {
			*end = ent;
			end = &ent->next;
		}
	}
	if (hold && mdstat_fd == -1)
		mdstat_fd = dup(fileno(f));
	fclose(f);

	/* If we might want to start array,
	 * reverse the order, so that components comes before composites
	 */
	if (start) {
		rv = NULL;
		while (all) {
			struct mdstat_ent *e = all;
			all = all->next;
			e->next = rv;
			rv = e;
		}
	} else rv = all;
	return rv;
}

void mdstat_wait(int seconds)
{
	fd_set fds;
	struct timeval tm;
	FD_ZERO(&fds);
	if (mdstat_fd >= 0)
		FD_SET(mdstat_fd, &fds);
	tm.tv_sec = seconds;
	tm.tv_usec = 0;
	select(mdstat_fd >2 ? mdstat_fd+1:3, NULL, NULL, &fds, &tm);
}

int mddev_busy(int devnum)
{
	struct mdstat_ent *mdstat = mdstat_read(0, 0);
	struct mdstat_ent *me;

	for (me = mdstat ; me ; me = me->next)
		if (me->devnum == devnum)
			break;
	free_mdstat(mdstat);
	return me != NULL;
}
