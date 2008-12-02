/*
 * NVRAM variable manipulation (Linux user mode half)
 *
 * Copyright 2005, Broadcom Corporation
 * All Rights Reserved.
 *
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: nvram_linux.c,v 1.18 2005/05/16 12:35:03 honor Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <error.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <typedefs.h>
#include <bcmnvram.h>
#include <nvram_convert.h>
#include <shutils.h>
#include <utils.h>
#include <shared.h>

#define PATH_DEV_NVRAM "/dev/nvram"

/* Globals */
static int nvram_fd = -1;
static char *nvram_buf = NULL;

int nvram_init(void *unused)
{
	if ((nvram_fd = open(PATH_DEV_NVRAM, O_RDWR)) >= 0) {
		/* Map kernel string buffer into user space */
		if ((nvram_buf = mmap(NULL, NVRAM_SPACE, PROT_READ, MAP_SHARED, nvram_fd, 0)) != MAP_FAILED) {
			fcntl(nvram_fd, F_SETFD, FD_CLOEXEC);	// zzz
			return 0;
		}
		close(nvram_fd);
		nvram_fd = -1;
	}
 	perror(PATH_DEV_NVRAM);
	return errno;
}

char *nvram_get(const char *name)
{
	char tmp[100];
	char *value;
	size_t count = strlen(name) + 1;
	unsigned long *off = (unsigned long *)tmp;

	if (nvram_fd < 0) {
		if (nvram_init(NULL) != 0) return NULL;
	}

	if (count > sizeof(tmp)) {
		if ((off = malloc(count)) == NULL) return NULL;
	}

	/* Get offset into mmap() space */
	strcpy((char *) off, name);
	count = read(nvram_fd, off, count);

	if (count == sizeof(*off)) {
		value = &nvram_buf[*off];
	}
	else {
		value = NULL;
		if (count < 0) perror(PATH_DEV_NVRAM);
	}

	if (off != (unsigned long *)tmp) free(off);
	return value;
}

int nvram_getall(char *buf, int count)
{
	int r;
	
	if (count <= 0) return 0;

	*buf = 0;
	if (nvram_fd < 0) {
		if ((r = nvram_init(NULL)) != 0) return r;
	}
	r = read(nvram_fd, buf, count);
	if (r < 0) perror(PATH_DEV_NVRAM);
	return (r == count) ? 0 : r;
}

static int _nvram_set(const char *name, const char *value)
{
	size_t count = strlen(name) + 1;
	char tmp[100];
	char *buf = tmp;
	int ret;

	if (nvram_fd < 0) {
		if ((ret = nvram_init(NULL)) != 0) return ret;
	}

	/* Unset if value is NULL */
	if (value) count += strlen(value) + 1;

	if (count > sizeof(tmp)) {
		if ((buf = malloc(count)) == NULL) return -ENOMEM;
	}

	if (value) {
		sprintf(buf, "%s=%s", name, value);
	}
	else {
		strcpy(buf, name);
	}

	ret = write(nvram_fd, buf, count);
	if (ret < 0) perror(PATH_DEV_NVRAM);

	if (buf != tmp) free(buf);

	return (ret == count) ? 0 : ret;
}

int nvram_set(const char *name, const char *value)
{
	struct nvram_convert *v;

	for (v = nvram_converts; v->name; v++) {
		if (!strcmp(v->name, name)) {
			_nvram_set(v->wl0_name, value);
			break;
		}
	}
	
	if (strncmp(name, "wl_", 3) == 0) {
		char wl0[48];
		
		if (strlen(name) < 32) {
			sprintf(wl0, "wl0_%s", name + 3);
			_nvram_set(wl0, value);
		}
	}

	return _nvram_set(name, value);
}

int nvram_unset(const char *name)
{
	return _nvram_set(name, NULL);
}

int nvram_commit(void)
{
	int r = 0;

	if (wait_action_idle(10)) {
		if (nvram_fd < 0) {
			if ((r = nvram_init(NULL)) != 0) return r;
		}
		set_action(ACT_NVRAM_COMMIT);
//		nvram_unset("dirty");
		r = ioctl(nvram_fd, NVRAM_MAGIC, NULL);
		set_action(ACT_IDLE);
		if (r < 0) {
			perror(PATH_DEV_NVRAM);
			cprintf("commit: error\n");
		}
	}
	else {
		cprintf("commit: system busy\n");
	}

	return r;
}


/*
int file2nvram(char *filename, char *varname)
{
   FILE *fp;
   int c,count;
   int i=0,j=0;
   char mem[10000],buf[30000];

   if ( !(fp=fopen(filename,"rb") ))
        return 0;

   count=fread(mem,1,sizeof(mem),fp);
   fclose(fp);
   for (j=0;j<count;j++) {
        if  (i > sizeof(buf)-3 )
                break;
        c=mem[j];
        if (c >= 32 && c <= 126 && c != '\\' && c != '~')  {
                buf[i++]=(unsigned char) c;
        } else if (c==0) {
		buf[i++]='~';
        } else {
                buf[i++]='\\';
                sprintf(buf+i,"%02X",c);
                i+=2;
        }
   }
   if (i==0) return 0;
   buf[i]=0;
   //fprintf(stderr,"================ > file2nvram %s = [%s] \n",varname,buf);
   nvram_set(varname,buf);
   //nvram_commit(); //Barry adds for test

   return 0;
}

int nvram2file(char *varname, char *filename) {
   FILE *fp;
   int c,tmp;
   int i=0,j=0;
   char *buf;
   char mem[10000];

   if ( !(fp=fopen(filename,"wb") ))
        return 0;

   buf=strdup(nvram_safe_get(varname));
   //fprintf(stderr,"=================> nvram2file %s = [%s] \n",varname,buf);
   while (  buf[i] && j < sizeof(mem)-3 ) {
        if (buf[i] == '\\')  {
                i++;
                tmp=buf[i+2];
                buf[i+2]=0;
                sscanf(buf+i,"%02X",&c);
                buf[i+2]=tmp;
                i+=2;
                mem[j]=c;j++;
        } else if (buf[i] == '~') {
		mem[j]=0;j++;
		i++;
        } else {
                mem[j]=buf[i];j++;
                i++;
        }
   }
   if (j<=0) return j;
   j=fwrite(mem,1,j,fp);
   fclose(fp);
   free(buf);
   return j;
}
*/
