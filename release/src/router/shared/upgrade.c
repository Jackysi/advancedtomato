
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

/*
 * Broadcom Home Gateway Reference Design
 * Web Page Configuration Support Routines
 *
 * Copyright 2001-2003, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 * $Id: upgrade.c,v 1.23 2005/05/05 03:36:32 honor Exp $
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

#ifdef MULTILANG_SUPPORT
	#define MIN_BUF_SIZE    1024
#else
	#define MIN_BUF_SIZE    4096
#endif

#define CODE_PATTERN_ERROR 9999
#define RESTORE	1
#define	CODE	2
static int upgrade_ret;

#ifdef WCN_SUPPORT
extern int xml_parser(char *input, int length);
extern int create_xml_output_file(void);
extern char output_file_name[];
#endif

void
//do_upgrade_cgi(char *url, FILE *stream)
do_upgrade_cgi(char *url, webs_t stream) //jimmy, https, 8/6/2003
{

	if(upgrade_ret)
//        		do_ej("Fail_u_s.asp",stream);
        		do_ej("/tmp/Fail_u_s.asp",stream);
	else
//        		do_ej("Success_u_s.asp",stream);
        		do_ej("/tmp/Success_u_s.asp",stream);

	websDone(stream, 200);

	/* Reboot if successful */
	if (upgrade_ret == 0){
		sleep(1);
		sys_reboot();
	}
}

int
//sys_upgrade(char *url, FILE *stream, int *total)
sys_upgrade(char *url, webs_t stream, int *total, int type) //jimmy, https, 8/6/2003
{
	char upload_fifo[] = "/tmp/uploadXXXXXX";
	FILE *fifo = NULL;
#ifdef MULTILANG_SUPPORT
	FILE *lang_fifo = NULL;
	int find_lang = 0;
	struct lang_header *pattern2=NULL;
	int lang_len = 0;
	int only_lang = 0;
	pid_t pid2;
	int ret2 = 0;
#endif
	char *write_argv[4];
	pid_t pid;
	char *buf = NULL;
	int count, ret = 0;
	long flags = -1;
	int size = BUFSIZ;
	int i=0;
	
#ifdef BACKUP_RESTORE_SUPPORT
	if(type == RESTORE){
		write_argv[0] = "restore";
		write_argv[1] = upload_fifo;
		write_argv[2] = "nvram";
		write_argv[3] = NULL;
	}else
#endif
	{
		write_argv[0] = "write";
		write_argv[1] = upload_fifo;
		write_argv[2] = "linux";
		write_argv[3] = NULL;
	}	

	if (url)
		return eval("write", url, "linux");
	
	//diag_led(DIAG, START_LED);  // blink the diag led
	C_led(1);
#ifdef HTTPS_SUPPORT
	if(do_ssl)
		ACTION("ACT_WEBS_UPGRADE");
	else
#endif
		ACTION("ACT_WEB_UPGRADE");
	

	/* Feed write from a temporary FIFO */
	if (!mktemp(upload_fifo) ||
		mkfifo(upload_fifo, S_IRWXU) < 0||
		(ret = _eval(write_argv, NULL, 0, &pid)) ||
		!(fifo = fopen(upload_fifo, "w"))) {
			if (!ret)
				ret = errno;
			goto err;
	}

	/* Set nonblock on the socket so we can timeout */
	if(!do_ssl){
		if ((flags = fcntl(fileno(stream), F_GETFL)) < 0 ||
			fcntl(fileno(stream), F_SETFL, flags | O_NONBLOCK) < 0) {
				ret = errno;
				goto err;
		}
	}

	/*
	 ** The buffer must be at least as big as what the stream file is
	 ** using so that it can read all the data that has been buffered 
	 ** in the stream file. Otherwise it would be out of sync with fn
	 ** select specially at the end of the data stream in which case
	 ** the select tells there is no more data available but there in 
	 ** fact is data buffered in the stream file's buffer. Since no
	 ** one has changed the default stream file's buffer size, let's
	 ** use the constant BUFSIZ until someone changes it.
	 **/

	if (size < MIN_BUF_SIZE)
                size = MIN_BUF_SIZE;
#ifdef MULTILANG_SUPPORT
                size = MIN_BUF_SIZE;
		cprintf("Upgrade buf size = [%d]\n", size);
#endif
        if ((buf = malloc(size)) == NULL) {
                ret = ENOMEM;
                goto err;
        }

	 /* Pipe the rest to the FIFO */
	cprintf("Upgrading\n");
	while (total && *total) {
#ifdef HTTPS_SUPPORT
		if(do_ssl){
			if (size > *total) size=*total;
			count = wfread(buf, 1, size, stream);
		}else
#endif
		{
			if (waitfor(fileno(stream), 5) <= 0){
			         cprintf("waitfor timeout 5 secs\n");
			         break;
			}
			count = safe_fread(buf, 1, size, stream);
	                if (!count && (ferror(stream) || feof(stream)))
				                        break;
		}
		
		if(i==0) {	// check code pattern, the first data must have code pattern
			char ver[40];
			long ver1, ver2, ver3;
			struct code_header *pattern = (struct code_header *)buf;
			
#ifdef BACKUP_RESTORE_SUPPORT
			if(type == RESTORE) {
				if((check_hw_type() == BCM4702_CHIP && pattern->hw_ver != 0) ||
				   (check_hw_type() == BCM4712_CHIP && pattern->hw_ver != 1) ||
				   (check_hw_type() == BCM5325E_CHIP && pattern->hw_ver != 2) ||
				   (check_hw_type() == BCM4704_BCM5325F_CHIP && pattern->hw_ver != 3) ||
				   (check_hw_type() == BCM5352E_CHIP && pattern->hw_ver != 4)) {
					cprintf("Cann't restore this configuration file to this HW\n");
					goto write_data;
				}
			}
#endif
				
			snprintf(ver, sizeof(ver), "v%d.%d.%d", pattern->fwvern[0], pattern->fwvern[1], pattern->fwvern[2]);
			ver1 = convert_ver(ver);
			ver2 = convert_ver(INTEL_FLASH_SUPPORT_VERSION_FROM);
			ver3 = convert_ver(BCM4712_CHIP_SUPPORT_VERSION_FROM);

			cprintf("upgrade_ver[%s] upgrade_ver[%ld] intel_ver[%ld] 4712_ver[%ld]\n", ver, ver1, ver2, ver3);
			
			if(memcmp(&buf[0], &CODE_PATTERN, 4)){
                        	cprintf("code pattern error!\n");
				goto write_data;
			}
#ifdef MULTILANG_SUPPORT
			else
			{
				pattern2 = (struct code_header *)buf;	
				if (pattern2->res2 == 'L')
				{
	                	       	cprintf("it's lang.bin\n");
					pattern2 = NULL;
					only_lang =1;
					goto lang;
				}
				else 	
	                	       	cprintf("it's code.bin\n");
				pattern2 = NULL;
//				goto err;
			}
#endif
			if(type != RESTORE && check_flash()){
				if(ver1 == -1 ||
				   ver2 == -1 ||
				   ver1 < ver2) {
					cprintf("The old firmware version cann't support intel flash\n");
					cprintf("Cann't downgrade to this old firmware version (%s), must be above %s(included)\n", ver, INTEL_FLASH_SUPPORT_VERSION_FROM);
					goto write_data;
				}
			}

			if(check_hw_type() == BCM4712_CHIP && 
			   ver1 < ver3){
				cprintf("The old firmware version cann't support bcm4712 chipset\n");
				cprintf("Cann't downgrade to this old firmware version (%s), must be above %s(included)\n", ver, BCM4712_CHIP_SUPPORT_VERSION_FROM);
				goto write_data;
			}

			if(check_hw_type() == BCM5325E_CHIP && !(pattern->flags & SUPPORT_5325E_SWITCH)) {
				cprintf("The old firmware version cann't support BCM5325E chipset\n");
				goto write_data;
			}

			if(check_hw_type() == BCM4704_BCM5325F_CHIP && !(pattern->flags & SUPPORT_4704_CHIP)) {
				cprintf("The old firmware version cann't support BCM4704 chipset\n");
				goto write_data;
			}

			if(check_hw_type() == BCM5352E_CHIP && !(pattern->flags & SUPPORT_5352E_CHIP)) {
				cprintf("The old firmware version cann't support BCM5352E chipset\n");
				goto write_data;
			}
			
			cprintf("code pattern correct!\n");
			*total -= count;
			safe_fwrite(&buf[sizeof(struct code_header)], 1, count-sizeof(struct code_header), fifo);
				
			i++;
			continue;
		}
write_data:
#ifdef MULTILANG_SUPPORT
		if( memcmp(&buf[0], &CODE_PATTERN, 4) == 0 ){
lang:
			find_lang = 1;
			pattern2 = (struct lang_header *)buf;	
			lang_len = pattern2->len;
                       	cprintf("found lang code pattern at block[%d]! len=[%d]\n",i, lang_len);
			lang_fifo = fopen("/tmp/lang.bin", "w");
			safe_fwrite(&buf[sizeof(struct lang_header)], 1, count-sizeof(struct lang_header), lang_fifo);
			*total -= count;
			lang_len = lang_len - count + sizeof(struct lang_header);
			i++;
			continue;
		}
//----------------------------------------------------------------------------------
		if(lang_len > 0)
		{
			safe_fwrite(buf, 1, MIN(count, lang_len), lang_fifo);
			lang_len = lang_len - count;
			*total -= count;
			cprintf(".");
			i++;
			if(lang_len < 0)
			{
	                       	cprintf("lang_len < 0 \n");
				goto lang2;
			}
				
//			if( (lang_len - count) < 0 )
				
			continue;
		}
		else if(lang_len < 0)
		{
                       	cprintf("create /tmp/lang.bin finished \n");
			goto lang2;
		}
			
		else
#endif	
//		*total -= count;
		safe_fwrite(buf, 1, count, fifo);
		*total -= count;
		cprintf(".");
		i++;
	}
	fclose(fifo);
	fifo = NULL;

	/* Wait for write to terminate */
	waitpid(pid, &ret, 0);
#ifdef MULTILANG_SUPPORT
lang2:
	if(!only_lang)
		waitpid(pid, &ret, 0);
	if( find_lang == 1)
	{
		fclose(lang_fifo);
		lang_fifo = NULL;
		write_argv[0] = "write";
		write_argv[1] = "/tmp/lang.bin";
		write_argv[2] = "lang";
		write_argv[3] = NULL;
/* if want to support web upgrade lang.bin please remove this mark 
		_eval(write_argv, NULL, 0, &pid2);
		waitpid(pid2, &ret2, 0);
*/
#ifdef WEB_UPGRADE_LANGPACK_SUPPORT
		_eval(write_argv, NULL, 0, &pid2);
		waitpid(pid2, &ret2, 0);
#endif
		unlink("/tmp/lang.bin");
	}
	else  
	{
		write_argv[0] = "erase";
		write_argv[1] = "lang";
		write_argv[2] = NULL;
		_eval(write_argv, NULL, 0, &pid2);
		waitpid(pid2, &ret2, 0);
	}	
#endif	
	cprintf("done\n");
	
	if(!do_ssl){
		/* Reset nonblock on the socket */
		if (fcntl(fileno(stream), F_SETFL, flags) < 0) {
			ret = errno;
			goto err;
		}
	}
	
err:
	if (buf)
		free(buf);
	if (fifo)
		fclose(fifo);
	unlink(upload_fifo);
	
	//diag_led(DIAG, STOP_LED);
	C_led(0);
	ACTION("ACT_IDLE");


	return ret;
}

												

void
//do_upgrade_post(char *url, FILE *stream, int len, char *boundary)
do_upgrade_post(char *url, webs_t stream, int len, char *boundary) //jimmy, https, 8/6/2003
{
	char buf[1024];
	int type = 0;

	upgrade_ret = EINVAL;
	
	// Let below files loaded to memory
	// To avoid the successful screen is blank after web upgrade.
//	system("cat /www/Success_u_s.asp > /dev/null");
//	system("cat /www/Fail_u_s.asp > /dev/null");
	system("cp /www/Success_u_s.asp /tmp/.");
	system("cp /www/Fail_u_s.asp /tmp/.");

	/* Look for our part */
	while (len > 0) {
		if (!wfgets(buf, MIN(len + 1, sizeof(buf)), stream))
			return;
		len -= strlen(buf);
		if (!strncasecmp(buf, "Content-Disposition:", 20)){
			if(strstr(buf, "name=\"file\"")){	// upgrade image
				type = CODE;
				break;
			}
#ifdef BACKUP_RESTORE_SUPPORT
			else if(strstr(buf, "name=\"restore\"")){	// restore configuration
				type = RESTORE;
				break;
			}
#endif
		}
	}
	/* Skip boundary and headers */
	while (len > 0) {
		if (!wfgets(buf, MIN(len + 1, sizeof(buf)), stream))
			return;
		len -= strlen(buf);
		if (!strcmp(buf, "\n") || !strcmp(buf, "\r\n"))
			break;
	}
	upgrade_ret = sys_upgrade(NULL, stream, &len, type);

	/* Slurp anything remaining in the request */

	while (len--)
#if defined(HTTPS_SUPPORT)
	     if(do_ssl)	
		     BIO_gets((BIO *)stream,buf,1);
	     else
#endif
	    	     (void) fgetc(stream);
					       

}
 
#ifdef WCN_SUPPORT
void
do_wcn_cgi(char *url, webs_t stream)
{
                                                                                                                                               
        cprintf("\nIn do_wcn_cgi(OUT)!!!\n");
                                                                                                                                               
        cprintf("\nOUTPUT XML FILE = %s\n", output_file_name);
        do_file(output_file_name, stream);
                                                                                                                                               
        sleep(1);
        sys_reboot();
        return;
                                                                                                                                               
                                                                                                                                               
        if(upgrade_ret)
        	do_ej("Fail_u_s.asp",stream);
        else
                do_ej("Success_u_s.asp",stream);
                                                                                                                                               
        websDone(stream, 200);
        return;
}

int
sys_wcn(char *url, webs_t stream, int *total, int type)
{
        char upload_fifo[] = "/tmp/uploadXXXXXX";
        FILE *fifo = NULL;
        char *write_argv[4];
        pid_t pid;
        char *buf = NULL;
        int count, ret = 0;
        long flags = -1;
        int size = BUFSIZ;
        int i=0;
        int income;
        char buffer[1024];
        struct code_header *pattern;
                                                                                                                                               
        bzero(buffer, 1024);
        income = *(total);
        cprintf("income = %d\n",income);
        //wfgets(buffer, 100, stream);
        wfread(buffer, 1, income, stream);
        *total = (*(total)-income);
        cprintf("total = %d\n",*total);
        cprintf("\nIn sys_wcn(IN)!!!\nDate:");
        for(i=0; i< income; i++)
        {
            //if((i%20)==0)
            //  cprintf("\n");
            if((buffer[i]!=0xd)&&(buffer[i]!=0xa))
                cprintf("%c",buffer[i]);
        }
        i =  xml_parser(buffer, income); //Added by Daniel(2004-11-01)
        i =  create_xml_output_file(); //Added by Daniel(2004-11-09)
        nvram_set("eou_configured", "1");
        nvram_set("ses_event","0"); // 2005/03/14 jeff add
	nvram_commit();
        cprintf("\nDate END!!!\n");
        return 0;                                                                                                                              }

void
//do_upgrade_post(char *url, FILE *stream, int len, char *boundary)
do_wcn_post(char *url, webs_t stream, int len, char *boundary) //Daniel
{
        char buf[1024];
        int type = 0;
                                                                                                                                               
        upgrade_ret = EINVAL;
                                                                                                                                               
        cprintf("\nIn do_wcn_post(IN)!!!\n");
                                                                                                                                               
        // Let below files loaded to memory
        // To avoid the successful screen is blank after web upgrade.
        system("cat /www/Success_u_s.asp > /dev/null");
        system("cat /www/Fail_u_s.asp > /dev/null");
                                                                                                                                               
        /* Look for our part */
        while (len > 0) {
                if (!wfgets(buf, MIN(len + 1, sizeof(buf)), stream))
                        return;
                len -= strlen(buf);
                if (!strncasecmp(buf, "Content-Disposition:", 20)){
                        if(strstr(buf, "name=\"file\"")){       // upgrade image
                                type = CODE;
                                break;
                        }
#ifdef BACKUP_RESTORE_SUPPORT
                        else if(strstr(buf, "name=\"restore\"")){       // restore configuration
                                type = RESTORE;
                                break;
                        }
#endif
                }
        }
        /* Skip boundary and headers */
        while (len > 0) {
                if (!wfgets(buf, MIN(len + 1, sizeof(buf)), stream))
                        return;
                len -= strlen(buf);
                if (!strcmp(buf, "\n") || !strcmp(buf, "\r\n"))
                        break;
        }
        cprintf("\n(Bf sys_wcn)len = %d\n",len);
        upgrade_ret = sys_wcn(NULL, stream, &len, type);
        cprintf("\n(Af sys_wcn)len = %d\n",len);
                                                                                                                                               
        /* Slurp anything remaining in the request */
                                                                                                                                               
        while (len--)
                     (void) fgetc(stream);
                                                                                                                                               
                                                                                                                                               
}

#endif


