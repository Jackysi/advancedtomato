/*
 * mapfile - manage /var/run/mdadm.map. Part of:
 * mdadm - manage Linux "md" devices aka RAID arrays.
 *
 * Copyright (C) 2006 Neil Brown <neilb@suse.de>
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
 *    Email: <neilb@suse.de>
 *    Paper: Neil Brown
 *           Novell Inc
 *           GPO Box Q1283
 *           QVB Post Office, NSW 1230
 *           Australia
 */

/* /var/run/mdadm.map is used to track arrays being created in --incremental
 * more.  It particularly allows lookup from UUID to array device, but
 * also allows the array device name to be easily found.
 *
 * The map file is line based with space separated fields.  The fields are:
 *  Device id  -  mdX or mdpX  where is a number.
 *  metadata   -  0.90 1.0 1.1 1.2
 *  UUID       -  uuid of the array
 *  path       -  path where device created: /dev/md/home
 *
 */


#include "mdadm.h"


int map_write(struct map_ent *mel)
{
	FILE *f;
	int err;
	int subdir = 1;

	f = fopen("/var/run/mdadm/map.new", "w");
	if (!f) {
		f = fopen("/var/run/mdadm.map.new", "w");
		subdir = 1;
	}
	if (!f)
		return 0;
	while (mel) {
		if (mel->devnum < 0)
			fprintf(f, "mdp%d ", -1-mel->devnum);
		else
			fprintf(f, "md%d ", mel->devnum);
		fprintf(f, "%d.%d ", mel->major, mel->minor);
		fprintf(f, "%08x:%08x:%08x:%08x ", mel->uuid[0],
			mel->uuid[1], mel->uuid[2], mel->uuid[3]);
		fprintf(f, "%s\n", mel->path);
		mel = mel->next;
	}
	fflush(f);
	err = ferror(f);
	fclose(f);
	if (err) {
		if (subdir)
			unlink("/var/run/mdadm/map.new");
		else
			unlink("/var/run/mdadm.map.new");
		return 0;
	}
	if (subdir)
		return rename("/var/run/mdadm/map.new",
			      "/var/run/mdadm/map") == 0;
	else
		return rename("/var/run/mdadm.map.new",
			      "/var/run/mdadm.map") == 0;
}

void map_add(struct map_ent **melp,
	    int devnum, int major, int minor, int uuid[4], char *path)
{
	struct map_ent *me = malloc(sizeof(*me));

	me->devnum = devnum;
	me->major = major;
	me->minor = minor;
	memcpy(me->uuid, uuid, 16);
	me->path = strdup(path);
	me->next = *melp;
	*melp = me;
}

void map_read(struct map_ent **melp)
{
	FILE *f;
	char buf[8192];
	char path[200];
	int devnum, major, minor, uuid[4];
	char nam[4];

	*melp = NULL;

	f = fopen("/var/run/mdadm/map", "r");
	if (!f)
		f = fopen("/var/run/mdadm.map", "r");
	if (!f)
		return;

	while (fgets(buf, sizeof(buf), f)) {
		if (sscanf(buf, " md%1[p]%d %d.%d %x:%x:%x:%x %200s",
			   nam, &devnum, &major, &minor, uuid, uuid+1,
			   uuid+2, uuid+3, path) == 9) {
			if (nam[0] == 'p')
				devnum = -1 - devnum;
			map_add(melp, devnum, major, minor, uuid, path);
		}
	}
	fclose(f);
}

void map_free(struct map_ent *map)
{
	while (map) {
		struct map_ent *mp = map;
		map = mp->next;
		free(mp->path);
		free(mp);
	}
}

int map_update(struct map_ent **mpp, int devnum, int major, int minor,
	       int *uuid, char *path)
{
	struct map_ent *map, *mp;
	int rv;

	if (mpp && *mpp)
		map = *mpp;
	else
		map_read(&map);

	for (mp = map ; mp ; mp=mp->next)
		if (mp->devnum == devnum) {
			mp->major = major;
			mp->minor = minor;
			memcpy(mp->uuid, uuid, 16);
			free(mp->path);
			mp->path = strdup(path);
			break;
		}
	if (!mp)
		map_add(&map, devnum, major, minor, uuid, path);
	*mpp = NULL;
	rv = map_write(map);
	map_free(map);
	return rv;
}

void map_delete(struct map_ent **mapp, int devnum)
{
	struct map_ent *mp;

	if (*mapp == NULL)
		map_read(mapp);

	for (mp = *mapp; mp; mp = *mapp) {
		if (mp->devnum == devnum) {
			*mapp = mp->next;
			free(mp->path);
			free(mp);
		} else
			mapp = & mp->next;
	}
}

struct map_ent *map_by_uuid(struct map_ent **map, int uuid[4])
{
	struct map_ent *mp;
	if (!*map)
		map_read(map);

	for (mp = *map ; mp ; mp = mp->next)
		if (memcmp(uuid, mp->uuid, 16) == 0)
			return mp;
	return NULL;

}
