
/*
 *********************************************************
 *   Copyright 2003, CyberTAN  Inc.  All Rights Reserved *
 *********************************************************

 This is UNPUBLISHED PROPRIETARY SOURCE CODE of CyberTAN Inc.
 the contents of this file may not be disclosed to third parties,
 copied or duplicated in any form without the prior written
 permission of CyberTAN Inc.

 This software should be used as a reference only, and it not
 intended for production use!


 THIS SOFTWARE IS OFFERED "AS IS", AND CYBERTAN GRANTS NO WARRANTIES OF ANY
 KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE.  CYBERTAN
 SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE
*/

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#include <broadcom.h>
#include <cyutils.h>
#include <code_pattern.h>

extern struct code_header *init_code_header(void);

void
do_backup(char *path, webs_t stream) 
{
	FILE *fp_r, *fp_w;
	char buf[NVRAM_SPACE];
	struct code_header *pattern;

	memset(&buf, 0, sizeof(buf));
	
	pattern = malloc(sizeof(struct code_header));
        memset(pattern, 0, sizeof(struct code_header));

	pattern = (struct code_header *)init_code_header();

	/* Write code pattern */ 

        /* Open NVRAM file */
        if (!(fp_r = fopen("/dev/mtd/3", "r"))) {
                perror("/dev/mtd/3");
                return;
        }
        fclose(fp_r);
        if (!(fp_r = fopen("/dev/mtd/4", "r"))) {
                perror("/dev/mtd/4");
                return;
        }

        /* Read NVRAM data */
        fseek(fp_r, -NVRAM_SPACE, SEEK_END);
       	fread(buf, NVRAM_SPACE, 1, fp_r);
        fclose(fp_r);
	
	encode(buf, sizeof(buf));
	
	if(!(fp_w = fopen("/tmp/config.bin", "w"))){
		perror("/tmp/config.bin");
		if(pattern)	free(pattern);
		return ;
	}

	/* Write code pattern */
	/* Due to the 4702/4712/4712L/4704 use different boot, restoring different backup files into different HW version, will cause Router crash */
	if(check_hw_type() == BCM5352E_CHIP)
		pattern->hw_ver = 4;
	else if(check_hw_type() == BCM4704_BCM5325F_CHIP)
		pattern->hw_ver = 3;
	else if(check_hw_type() == BCM5325E_CHIP)
		pattern->hw_ver = 2;
	else if(check_hw_type() == BCM4712_CHIP)
		pattern->hw_ver = 1;
	else
		pattern->hw_ver = 0;
		
	fwrite(pattern, 1, sizeof(struct code_header), fp_w);
	/*Write NVRAM data */
	fwrite(buf, 1, sizeof(buf), fp_w);
	fclose(fp_w);

        do_file("/tmp/config.bin", stream);

	if(pattern)	free(pattern);
}

int
ej_get_backup_name(int eid, webs_t wp, int argc, char_t **argv)
{
	int ret;
	char buf[80];
	
	if(check_hw_type() == BCM4702_CHIP)
		snprintf(buf, sizeof(buf), "%sV1_%s.cfg", MODEL_NAME, CYBERTAN_VERSION);
	else if((check_hw_type() == BCM4712_CHIP) || (check_hw_type() == BCM5325E_CHIP))
		snprintf(buf, sizeof(buf), "%sV2_%s.cfg", MODEL_NAME, CYBERTAN_VERSION);
	else if(check_hw_type() == BCM4704_BCM5325F_CHIP)
		snprintf(buf, sizeof(buf), "%sV5_%s.cfg", MODEL_NAME, CYBERTAN_VERSION);
	else if(check_hw_type() == BCM5352E_CHIP)
		snprintf(buf, sizeof(buf), "%s%s_%s.cfg", MODEL_NAME, MODEL_VERSION, CYBERTAN_VERSION);
	else
		snprintf(buf, sizeof(buf), "%s_%s.cfg", MODEL_NAME, CYBERTAN_VERSION);

	ret = websWrite(wp, "%s", buf);

	return ret;
}	

int
ej_view_config(int eid, webs_t wp, int argc, char_t **argv)
{
	int ret = 0;
        FILE *fp;
        char line[10240];
	
	if ((fp = popen("nvram show | sort", "r")) != NULL) {
		while( fgets(line, sizeof(line), fp) != NULL ) {
			line[strlen(line)-1] = '\0';
			if(!strcmp(line,""))	continue;

			ret += websWrite(wp, "%s<br>\n", line);
		}
		pclose(fp);
	}

	return ret;
}
