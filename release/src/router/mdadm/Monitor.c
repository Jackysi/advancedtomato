/*
 * mdadm - manage Linux "md" devices aka RAID arrays.
 *
 * Copyright (C) 2001-2006 Neil Brown <neilb@suse.de>
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

#include	"mdadm.h"
#include	"md_p.h"
#include	"md_u.h"
#include	<sys/wait.h>
#include	<sys/signal.h>
#include	<values.h>
#include	<syslog.h>

static void alert(char *event, char *dev, char *disc, char *mailaddr, char *mailfrom,
		  char *cmd, int dosyslog);

static char *percentalerts[] = { 
	"RebuildStarted",
	"Rebuild20",
	"Rebuild40",
	"Rebuild60",
	"Rebuild80",
};

/* The largest number of disks current arrays can manage is 384
 * This really should be dynamically, but that will have to wait
 * At least it isn't MD_SB_DISKS.
 */
#define MaxDisks 384
int Monitor(mddev_dev_t devlist,
	    char *mailaddr, char *alert_cmd,
	    int period, int daemonise, int scan, int oneshot,
	    int dosyslog, int test, char* pidfile)
{
	/*
	 * Every few seconds, scan every md device looking for changes
	 * When a change is found, log it, possibly run the alert command,
	 * and possibly send Email
	 *
	 * For each array, we record:
	 *   Update time
	 *   active/working/failed/spare drives
	 *   State of each device.
	 *   %rebuilt if rebuilding
	 *
	 * If the update time changes, check out all the data again
	 * It is possible that we cannot get the state of each device
	 * due to bugs in the md kernel module.
	 * We also read /proc/mdstat to get rebuild percent,
	 * and to get state on all active devices incase of kernel bug.
	 *
	 * Events are:
	 *    Fail
	 *	An active device had Faulty set or Active/Sync removed
	 *    FailSpare
	 *      A spare device had Faulty set
	 *    SpareActive
	 *      An active device had a reverse transition
	 *    RebuildStarted
	 *      percent went from -1 to +ve
	 *    Rebuild20 Rebuild40 Rebuild60 Rebuild80
	 *      percent went from below to not-below that number
	 *    DeviceDisappeared
	 *      Couldn't access a device which was previously visible
	 *
	 * if we detect an array with active<raid and spare==0
	 * we look at other arrays that have same spare-group
	 * If we find one with active==raid and spare>0,
	 *  and if we can get_disk_info and find a name
	 *  Then we hot-remove and hot-add to the other array
	 *
	 * If devlist is NULL, then we can monitor everything because --scan
	 * was given.  We get an initial list from config file and add anything
	 * that appears in /proc/mdstat
	 */

	struct state {
		char *devname;
		int devnum;	/* to sync with mdstat info */
		long utime;
		int err;
		char *spare_group;
		int active, working, failed, spare, raid;
		int expected_spares;
		int devstate[MaxDisks];
		int devid[MaxDisks];
		int percent;
		struct state *next;
	} *statelist = NULL;
	int finished = 0;
	struct mdstat_ent *mdstat = NULL;
	char *mailfrom = NULL;

	if (!mailaddr) {
		mailaddr = conf_get_mailaddr();
		if (mailaddr && ! scan)
			fprintf(stderr, Name ": Monitor using email address \"%s\" from config file\n",
			       mailaddr);
	}
	mailfrom = conf_get_mailfrom();

	if (!alert_cmd) {
		alert_cmd = conf_get_program();
		if (alert_cmd && ! scan)
			fprintf(stderr, Name ": Monitor using program \"%s\" from config file\n",
			       alert_cmd);
	}
	if (scan && !mailaddr && !alert_cmd) {
		fprintf(stderr, Name ": No mail address or alert command - not monitoring.\n");
		return 1;
	}

	if (daemonise) {
		int pid = fork();
		if (pid > 0) {
			if (!pidfile)
				printf("%d\n", pid);
			else {
				FILE *pid_file;
				pid_file=fopen(pidfile, "w");
				if (!pid_file)
					perror("cannot create pid file");
				else {
					fprintf(pid_file,"%d\n", pid);
					fclose(pid_file);
				}
			}
			return 0;
		}
		if (pid < 0) {
			perror("daemonise");
			return 1;
		}
		close(0);
		open("/dev/null", 3);
		dup2(0,1);
		dup2(0,2);
		setsid();
	}

	if (devlist == NULL) {
		mddev_ident_t mdlist = conf_get_ident(NULL);
		for (; mdlist; mdlist=mdlist->next) {
			struct state *st = malloc(sizeof *st);
			if (st == NULL)
				continue;
			st->devname = strdup(mdlist->devname);
			st->utime = 0;
			st->next = statelist;
			st->err = 0;
			st->devnum = MAXINT;
			st->percent = -2;
			st->expected_spares = mdlist->spare_disks;
			if (mdlist->spare_group)
				st->spare_group = strdup(mdlist->spare_group);
			else
				st->spare_group = NULL;
			statelist = st;
		}
	} else {
		mddev_dev_t dv;
		for (dv=devlist ; dv; dv=dv->next) {
			mddev_ident_t mdlist = conf_get_ident(dv->devname);
			struct state *st = malloc(sizeof *st);
			if (st == NULL)
				continue;
			st->devname = strdup(dv->devname);
			st->utime = 0;
			st->next = statelist;
			st->err = 0;
			st->devnum = MAXINT;
			st->percent = -2;
			st->expected_spares = -1;
			st->spare_group = NULL;
			if (mdlist) {
				st->expected_spares = mdlist->spare_disks;
				if (mdlist->spare_group)
					st->spare_group = strdup(mdlist->spare_group);
			}
			statelist = st;
		}
	}


	while (! finished) {
		int new_found = 0;
		struct state *st;

		if (mdstat)
			free_mdstat(mdstat);
		mdstat = mdstat_read(oneshot?0:1, 0);

		for (st=statelist; st; st=st->next) {
			struct { int state, major, minor; } info[MaxDisks];
			mdu_array_info_t array;
			struct mdstat_ent *mse = NULL, *mse2;
			char *dev = st->devname;
			int fd;
			unsigned int i;

			if (test)
				alert("TestMessage", dev, NULL, mailaddr, mailfrom, alert_cmd, dosyslog);
			fd = open(dev, O_RDONLY);
			if (fd < 0) {
				if (!st->err)
					alert("DeviceDisappeared", dev, NULL,
					      mailaddr, mailfrom, alert_cmd, dosyslog);
/*					fprintf(stderr, Name ": cannot open %s: %s\n",
						dev, strerror(errno));
*/				st->err=1;
				continue;
			}
			if (ioctl(fd, GET_ARRAY_INFO, &array)<0) {
				if (!st->err)
					alert("DeviceDisappeared", dev, NULL,
					      mailaddr, mailfrom, alert_cmd, dosyslog);
/*					fprintf(stderr, Name ": cannot get array info for %s: %s\n",
						dev, strerror(errno));
*/				st->err=1;
				close(fd);
				continue;
			}
			if (array.level != 1 && array.level != 5 && array.level != -4 &&
				array.level != 6 && array.level != 10) {
				if (!st->err)
					alert("DeviceDisappeared", dev, "Wrong-Level",
					      mailaddr, mailfrom, alert_cmd, dosyslog);
				st->err = 1;
				close(fd);
				continue;
			}
			if (st->devnum == MAXINT) {
				struct stat stb;
				if (fstat(fd, &stb) == 0 &&
				    (S_IFMT&stb.st_mode)==S_IFBLK) {
					if (major(stb.st_rdev) == MD_MAJOR)
						st->devnum = minor(stb.st_rdev);
					else
						st->devnum = -1- (minor(stb.st_rdev)>>6);
				}
			}

			for (mse2 = mdstat ; mse2 ; mse2=mse2->next)
				if (mse2->devnum == st->devnum) {
					mse2->devnum = MAXINT; /* flag it as "used" */
					mse = mse2;
				}

			if (st->utime == array.utime &&
			    st->failed == array.failed_disks &&
			    st->working == array.working_disks &&
			    st->spare == array.spare_disks &&
			    (mse == NULL  || (
				    mse->percent == st->percent
				    ))) {
				close(fd);
				st->err = 0;
				continue;
			}
			if (st->utime == 0 && /* new array */
			    mse &&	/* is in /proc/mdstat */
			    mse->pattern && strchr(mse->pattern, '_') /* degraded */
				)
				alert("DegradedArray", dev, NULL, mailaddr, mailfrom, alert_cmd, dosyslog);

			if (st->utime == 0 && /* new array */
			    st->expected_spares > 0 && 
			    array.spare_disks < st->expected_spares) 
				alert("SparesMissing", dev, NULL, mailaddr, mailfrom, alert_cmd, dosyslog);
			if (mse &&
			    st->percent == -1 && 
			    mse->percent >= 0)
				alert("RebuildStarted", dev, NULL, mailaddr, mailfrom, alert_cmd, dosyslog);
			if (mse &&
			    st->percent >= 0 &&
			    mse->percent >= 0 &&
			    (mse->percent / 20) > (st->percent / 20))
				alert(percentalerts[mse->percent/20],
				      dev, NULL, mailaddr, mailfrom, alert_cmd, dosyslog);

			if (mse &&
			    mse->percent == -1 &&
			    st->percent >= 0) {
				/* Rebuild/sync/whatever just finished.
				 * If there is a number in /mismatch_cnt,
				 * we should report that.
				 */
				struct sysarray *sra =
				       sysfs_read(-1, st->devnum, GET_MISMATCH);
				if (sra && sra->mismatch_cnt > 0) {
					char cnt[40];
					sprintf(cnt, " mismatches found: %d", sra->mismatch_cnt);
					alert("RebuildFinished", dev, cnt, mailaddr, mailfrom, alert_cmd, dosyslog);
				} else
					alert("RebuildFinished", dev, NULL, mailaddr, mailfrom, alert_cmd, dosyslog);
				if (sra)
					free(sra);
			}

			if (mse)
				st->percent = mse->percent;


			for (i=0; i<MaxDisks && i <= array.raid_disks + array.nr_disks;
			     i++) {
				mdu_disk_info_t disc;
				if (ioctl(fd, GET_DISK_INFO, &disc) >= 0) {
					info[i].state = disc.state;
					info[i].major = disc.major;
					info[i].minor = disc.minor;
				} else
					info[i].major = info[i].minor = 0;
			}
			close(fd);

			for (i=0; i<MaxDisks; i++) {
				mdu_disk_info_t disc = {0};
				int newstate=0;
				int change;
				char *dv = NULL;
				disc.number = i;
				if (i > array.raid_disks + array.nr_disks) {
					newstate = 0;
					disc.major = disc.minor = 0;
				} else if (info[i].major || info[i].minor) {
					newstate = info[i].state;
					dv = map_dev(info[i].major, info[i].minor, 1);
					disc.state = newstate;
					disc.major = info[i].major;
					disc.minor = info[i].minor;
				} else if (mse &&  mse->pattern && i < strlen(mse->pattern)) {
					switch(mse->pattern[i]) {
					case 'U': newstate = 6 /* ACTIVE/SYNC */; break;
					case '_': newstate = 0; break;
					}
					disc.major = disc.minor = 0;
				}
				if (dv == NULL && st->devid[i])
					dv = map_dev(major(st->devid[i]),
						     minor(st->devid[i]), 1);
				change = newstate ^ st->devstate[i];
				if (st->utime && change && !st->err) {
					if (i < (unsigned)array.raid_disks &&
					    (((newstate&change)&(1<<MD_DISK_FAULTY)) ||
					     ((st->devstate[i]&change)&(1<<MD_DISK_ACTIVE)) ||
					     ((st->devstate[i]&change)&(1<<MD_DISK_SYNC)))
						)
						alert("Fail", dev, dv, mailaddr, mailfrom, alert_cmd, dosyslog);
					else if (i >= (unsigned)array.raid_disks &&
						 (disc.major || disc.minor) &&
						 st->devid[i] == makedev(disc.major, disc.minor) &&
						 ((newstate&change)&(1<<MD_DISK_FAULTY))
						)
						alert("FailSpare", dev, dv, mailaddr, mailfrom, alert_cmd, dosyslog);
					else if (i < (unsigned)array.raid_disks &&
						 (((st->devstate[i]&change)&(1<<MD_DISK_FAULTY)) ||
						  ((newstate&change)&(1<<MD_DISK_ACTIVE)) ||
						  ((newstate&change)&(1<<MD_DISK_SYNC)))
						)
						alert("SpareActive", dev, dv, mailaddr, mailfrom, alert_cmd, dosyslog);
				}
				st->devstate[i] = disc.state;
				st->devid[i] = makedev(disc.major, disc.minor);
			}
			st->active = array.active_disks;
			st->working = array.working_disks;
			st->spare = array.spare_disks;
			st->failed = array.failed_disks;
			st->utime = array.utime;
			st->raid = array.raid_disks;
			st->err = 0;
		}
		/* now check if there are any new devices found in mdstat */
		if (scan) {
			struct mdstat_ent *mse;
			for (mse=mdstat; mse; mse=mse->next) 
				if (mse->devnum != MAXINT &&
				    (strcmp(mse->level, "raid1")==0 ||
				     strcmp(mse->level, "raid5")==0 ||
				     strcmp(mse->level, "multipath")==0)
					) {
					struct state *st = malloc(sizeof *st);
					mdu_array_info_t array;
					int fd;
					if (st == NULL)
						continue;
					st->devname = strdup(get_md_name(mse->devnum));
					if ((fd = open(st->devname, O_RDONLY)) < 0 ||
					    ioctl(fd, GET_ARRAY_INFO, &array)< 0) {
						/* no such array */
						if (fd >=0) close(fd);
						put_md_name(st->devname);
						free(st->devname);
						free(st);
						continue;
					}
					close(fd);
					st->utime = 0;
					st->next = statelist;
					st->err = 1;
					st->devnum = mse->devnum;
					st->percent = -2;
					st->spare_group = NULL;
					st->expected_spares = -1;
					statelist = st;
					alert("NewArray", st->devname, NULL, mailaddr, mailfrom, alert_cmd, dosyslog);
					new_found = 1;
				}
		}
		/* If an array has active < raid && spare == 0 && spare_group != NULL
		 * Look for another array with spare > 0 and active == raid and same spare_group
		 *  if found, choose a device and hotremove/hotadd
		 */
		for (st = statelist; st; st=st->next)
			if (st->active < st->raid &&
			    st->spare == 0 &&
			    st->spare_group != NULL) {
				struct state *st2;
				for (st2=statelist ; st2 ; st2=st2->next)
					if (st2 != st &&
					    st2->spare > 0 &&
					    st2->active == st2->raid &&
					    st2->spare_group != NULL &&
					    strcmp(st->spare_group, st2->spare_group) == 0) {
						/* try to remove and add */
						int fd1 = open(st->devname, O_RDONLY);
						int fd2 = open(st2->devname, O_RDONLY);
						int dev = -1;
						int d;
						if (fd1 < 0 || fd2 < 0) {
							if (fd1>=0) close(fd1);
							if (fd2>=0) close(fd2);
							continue;
						}
						for (d=st2->raid; d < MaxDisks; d++) {
							if (st2->devid[d] > 0 &&
							    st2->devstate[d] == 0) {
								dev = st2->devid[d];
								break;
							}
						}
						if (dev > 0) {
							if (ioctl(fd2, HOT_REMOVE_DISK, 
								  (unsigned long)dev) == 0) {
								if (ioctl(fd1, HOT_ADD_DISK,
									  (unsigned long)dev) == 0) {
									alert("MoveSpare", st->devname, st2->devname, mailaddr, mailfrom, alert_cmd, dosyslog);
									close(fd1);
									close(fd2);
									break;
								}
								else ioctl(fd2, HOT_ADD_DISK, (unsigned long) dev);
							}
						}
						close(fd1);
						close(fd2);
					}
			}
		if (!new_found) {
			if (oneshot)
				break;
			else
				mdstat_wait(period);
		}
		test = 0;
	}
	if (pidfile)
		unlink(pidfile);
	return 0;
}


static void alert(char *event, char *dev, char *disc, char *mailaddr, char *mailfrom, char *cmd,
		  int dosyslog)
{
	int priority;

	if (!cmd && !mailaddr) {
		time_t now = time(0);
	       
		printf("%1.15s: %s on %s %s\n", ctime(&now)+4, event, dev, disc?disc:"unknown device");
	}
	if (cmd) {
		int pid = fork();
		switch(pid) {
		default:
			waitpid(pid, NULL, 0);
			break;
		case -1:
			break;
		case 0:
			execl(cmd, cmd, event, dev, disc, NULL);
			exit(2);
		}
	}
	if (mailaddr && 
	    (strncmp(event, "Fail", 4)==0 || 
	     strncmp(event, "Test", 4)==0 ||
	     strncmp(event, "Spares", 6)==0 ||
	     strncmp(event, "Degrade", 7)==0)) {
		FILE *mp = popen(Sendmail, "w");
		if (mp) {
			FILE *mdstat;
			char hname[256];
			gethostname(hname, sizeof(hname));
			signal(SIGPIPE, SIG_IGN);
			if (mailfrom)
				fprintf(mp, "From: %s\n", mailfrom);
			else
				fprintf(mp, "From: " Name " monitoring <root>\n");
			fprintf(mp, "To: %s\n", mailaddr);
			fprintf(mp, "Subject: %s event on %s:%s\n\n", event, dev, hname);

			fprintf(mp, "This is an automatically generated mail message from " Name "\n");
			fprintf(mp, "running on %s\n\n", hname);

			fprintf(mp, "A %s event had been detected on md device %s.\n\n", event, dev);

			if (disc && disc[0] != ' ')
				fprintf(mp, "It could be related to component device %s.\n\n", disc);
			if (disc && disc[0] == ' ')
				fprintf(mp, "Extra information:%s.\n\n", disc);

			fprintf(mp, "Faithfully yours, etc.\n");

			mdstat = fopen("/proc/mdstat", "r");
			if (mdstat) {
				char buf[8192];
				int n;
				fprintf(mp, "\nP.S. The /proc/mdstat file currently contains the following:\n\n");
				while ( (n=fread(buf, 1, sizeof(buf), mdstat)) > 0)
					n=fwrite(buf, 1, n, mp); /* yes, i don't care about the result */
				fclose(mdstat);
			}
			fclose(mp);
		}

	}

	/* log the event to syslog maybe */
	if (dosyslog) {
		/* Log at a different severity depending on the event.
		 *
		 * These are the critical events:  */
		if (strncmp(event, "Fail", 4)==0 ||
		    strncmp(event, "Degrade", 7)==0 ||
		    strncmp(event, "DeviceDisappeared", 17)==0)
			priority = LOG_CRIT;
		/* Good to know about, but are not failures: */
		else if (strncmp(event, "Rebuild", 7)==0 ||
			 strncmp(event, "MoveSpare", 9)==0 ||
			 strncmp(event, "Spares", 6) != 0)
			priority = LOG_WARNING;
		/* Everything else: */
		else
			priority = LOG_INFO;

		if (disc)
			syslog(priority, "%s event detected on md device %s, component device %s", event, dev, disc);
		else
			syslog(priority, "%s event detected on md device %s", event, dev);
	}
}

/* Not really Monitor but ... */
int Wait(char *dev)
{
	struct stat stb;
	int devnum;
	int rv = 1;

	if (stat(dev, &stb) != 0) {
		fprintf(stderr, Name ": Cannot find %s: %s\n", dev,
			strerror(errno));
		return 2;
	}
	if (major(stb.st_rdev) == MD_MAJOR)
		devnum = minor(stb.st_rdev);
	else
		devnum = -1-(minor(stb.st_rdev)/64);

	while(1) {
		struct mdstat_ent *ms = mdstat_read(1, 0);
		struct mdstat_ent *e;

		for (e=ms ; e; e=e->next)
			if (e->devnum == devnum)
				break;

		if (!e || e->percent < 0) {
			free_mdstat(ms);
			return rv;
		}
		free(ms);
		rv = 0;
		mdstat_wait(5);
	}
}
