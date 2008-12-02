
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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <net/if.h>
#include <dirent.h>
#include <unistd.h>
#include <ctype.h>
#include <syslog.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <sys/sysinfo.h>

#include <utils.h>
#include <bcmnvram.h>
#include <shutils.h>
#include <cy_conf.h>
#include <code_pattern.h>
#include <bcmdevs.h>
#include <net/route.h>
#include <cy_conf.h>
#include <bcmdevs.h>

#include <linux/if_ether.h>
//#include <linux/mii.h>
#include <linux/sockios.h>

#define SIOCGMIIREG	0x8948          /* Read MII PHY register.       */
#define SIOCSMIIREG	0x8949          /* Write MII PHY register.      */

struct mii_ioctl_data {
	unsigned short		phy_id;
	unsigned short		reg_num;
	unsigned short		val_in;
	unsigned short		val_out;
};

int
diag_led_4702(int type, int act)
{
	FILE *fp;	
	char string[10];
	int c = 0;
	
	int leds[3][2]={ { 1 , 6},
		         { 2 , 5},
		         { 4 , 3} };

	if(type == DMZ)			type = 0;
	//else if(type == SESSION)	type = 1;
	else if(type == DIAG)		type = 2;
	else return 0;

	if( (fp=fopen("/proc/sys/diag", "r")) ){
		fgets(string, sizeof(string), fp);
		fclose(fp);
	}
	else{
		perror("/proc/sys/diag");
		return 0;
	}

	if(act == STOP_LED){
		c = (atoi(string) & leds[type][act]) + 48;
	}
	else if(act == START_LED){
		c = (atoi(string) | leds[type][act]) + 48;
	}
	fprintf(stderr, "diag led = [%d] -> [%c]\n",atoi(string), c);	

	if( (fp=fopen("/proc/sys/diag", "w")) ){
		fputc(c, fp);
		fclose(fp);
	}
	else
		perror("/proc/sys/diag");


	return 0;
}

int 
C_led_4702(int i)
{
        FILE *fp;
        char string[10];
        int flg;

        memset(string,0,10);
        /* get diag before set */
        if( (fp=fopen("/proc/sys/diag", "r")) ){
              fgets(string, sizeof(string), fp);
              fclose(fp);
        }
        else
              perror("/proc/sys/diag");

        if(i) flg=atoi(string) | 0x10 ;
        else flg=atoi(string) & 0xef;

        memset(string,0,10);
        sprintf(string,"%d",flg);
        if( (fp=fopen("/proc/sys/diag", "w")) ){
                fputs(string, fp);
                fclose(fp);
        }
        else
              perror("/proc/sys/diag");

        return 0;
}

unsigned int 
read_gpio(char *device)
{
	FILE *fp;
	unsigned int val;

        if( (fp=fopen(device, "r")) ){
            fread(&val, 4, 1, fp);
            fclose(fp);
	    //fprintf(stderr, "----- gpio %s = [%X]\n",device,val); 
	    return val;
        }
        else{
            perror(device);
            return 0;
        }
}

unsigned int 
write_gpio(char *device, unsigned int val)
{
        FILE *fp;
 
        if( (fp=fopen(device, "w")) ){
	    fwrite(&val, 4, 1, fp); 
            fclose(fp);
	    //fprintf(stderr, "----- set gpio %s = [%X]\n",device,val); 
            return 1;
        }
        else{
            perror(device);
            return 0;
        }
}

static char hw_error=0;
int
diag_led_4704(int type, int act)
{
	unsigned int control,in,outen,out;
	
#ifdef BCM94712AGR
	/* The router will crash, if we load the code into broadcom demo board. */
	return 1;
#endif

	control=read_gpio("/dev/gpio/control");
	in=read_gpio("/dev/gpio/in");
	out=read_gpio("/dev/gpio/out");
	outen=read_gpio("/dev/gpio/outen");	

	write_gpio("/dev/gpio/outen",(outen & 0x7c) | 0x83);
	switch (type) {
		case DIAG:	// GPIO 1
			if(hw_error){
				write_gpio("/dev/gpio/out",(out & 0x7c) | 0x00);
				return 1;
			}
	
			if (act == STOP_LED) { // stop blinking
				write_gpio("/dev/gpio/out",(out & 0x7c) | 0x83);
				//cprintf("tallest:=====( DIAG STOP_LED !!)=====\n");
			} else if (act == START_LED) { // start blinking
				write_gpio("/dev/gpio/out",(out & 0x7c) | 0x81);
				//cprintf("tallest:=====( DIAG START_LED !!)=====\n");
			} else if (act == MALFUNCTION_LED) { // start blinking
				write_gpio("/dev/gpio/out",(out & 0x7c) | 0x00);
				hw_error=1;
				//cprintf("tallest:=====( DIAG MALFUNCTION_LED !!)=====\n");
			}
			break;
	}	
	return 1;
}				

int
diag_led_4712(int type, int act)
{
	unsigned int control, in, outen, out, ctr_mask, out_mask;
	
#ifdef BCM94712AGR
	/* The router will crash, if we load the code into broadcom demo board. */
	return 1;
#endif
	control	=	read_gpio("/dev/gpio/control");
	in	=	read_gpio("/dev/gpio/in");
	out	=	read_gpio("/dev/gpio/out");
	outen	=	read_gpio("/dev/gpio/outen");

	ctr_mask=	~(1 << type);
	out_mask=	(1 << type);

	write_gpio("/dev/gpio/control", control & ctr_mask);
	write_gpio("/dev/gpio/outen", outen | out_mask);

	if (act == STOP_LED) { // stop blinking
		//cprintf("%s: Stop GPIO %d\n", __FUNCTION__, type);
		write_gpio("/dev/gpio/out", out | out_mask);
	} else if (act == START_LED) { // start blinking
		//cprintf("%s: Start GPIO %d\n", __FUNCTION__, type);
		write_gpio("/dev/gpio/out", out & ctr_mask);
	}

	return 1;
}				
	
int C_led_4712(int i)
{
	if (i==1) 
		return diag_led(DIAG,START_LED);
	else
		return diag_led(DIAG,STOP_LED);
}		

int 
C_led(int i)
{
	if(check_hw_type() == BCM4702_CHIP)
		return C_led_4702(i);
	else
		return C_led_4712(i);
}

int
diag_led(int type, int act)
{
	if(check_hw_type() == BCM4702_CHIP)
		return diag_led_4702(type, act);
	else if(check_hw_type() == BCM4704_BCM5325F_CHIP)
		return diag_led_4704(type, act);
	else
		return diag_led_4712(type, act);
}

char *
get_mac_from_ip(char *ip)
{
	FILE *fp;
        char line[100];

	char ipa[50];           // ip address
	char hwa[50];           // HW address / MAC
	char mask[50];          // ntemask   
	char dev[50];           // interface
	int type;               // HW type
	int flags;              // flags
	static char mac[20]; 
							

        if ((fp = fopen("/proc/net/arp", "r")) == NULL) return NULL;

        // Bypass header -- read until newline 
	if (fgets(line, sizeof(line), fp) != (char *) NULL) {
	      // Read the ARP cache entries.
	      // IP address       HW type     Flags       HW address            Mask     Device
	      // 192.168.1.1      0x1         0x2         00:90:4C:21:00:2A     *        eth0
		for (; fgets(line, sizeof(line), fp);) {
			if(sscanf(line, "%s 0x%x 0x%x %100s %100s %100s\n", ipa, &type, &flags, hwa, mask, dev)!=6)
				continue;
			//cprintf("ip1=[%s] ip2=[%s] mac=[%s] (flags & ATF_COM)=%d\n", ip, ipa, hwa, (flags & ATF_COM));
			if(strcmp(ip, ipa))
				continue;
			//if (!(flags & ATF_COM)) {       //ATF_COM = 0x02   completed entry (ha valid)
				strcpy(mac, hwa);
				fclose(fp);
				return mac;
			//}
		}
	}

	fclose(fp);
	return "";
}

struct dns_lists *
get_dns_list(int no)
{
        char list[254];
        char *next, word[254];
        struct dns_lists *dns_list = NULL;
	int i, match = 0;
	char got_dns[2][15]={"wan_get_dns","wan_get_dns_1"};

        if(!(dns_list = (struct dns_lists *)malloc(sizeof(struct dns_lists))))
			return NULL;

        memset(dns_list, 0, sizeof(struct dns_lists));

        dns_list->num_servers = 0;

	// nvram_safe_get("wan_dns") ==> Set by user
	// nvram_safe_get("wan_get_dns") ==> Get from DHCP, PPPoE or PPTP
	// The nvram_safe_get("wan_dns") priority is higher than nvram_safe_get("wan_get_dns")
        if(no == 0){
		snprintf(list, sizeof(list), "%s %s", nvram_safe_get("wan_dns"), nvram_safe_get(got_dns[no]));
	}
        foreach(word, list, next) {
                if(strcmp(word, "0.0.0.0") && strcmp(word, "")){
			match = 0;
			for(i=0 ; i<dns_list->num_servers ; i++){	// Skip same DNS
				if(!strcmp(dns_list->dns_server[i], word))	match = 1;
			}
			if(!match){
				snprintf(dns_list->dns_server[dns_list->num_servers], sizeof(dns_list->dns_server[dns_list->num_servers]), "%s", word);
				dns_list->num_servers ++ ;
			}
                }
                if(dns_list->num_servers == 3)      break;	// We only need 3 counts DNS entry
        }
        return dns_list;
}

int
dns_to_resolv(void)
{
	FILE *fp_w;
	struct dns_lists *dns_list = NULL;
	int i = 0;

	/* Save DNS to resolv.conf */
	if (!(fp_w = fopen(RESOLV_FILE, "w"))) {
                perror(RESOLV_FILE);
                return errno;
        }

	dns_list = get_dns_list(0);

        for(i=0 ; i<dns_list->num_servers ; i++)
                fprintf(fp_w, "nameserver %s\n", dns_list->dns_server[i]);
	
	/* Put a pseudo DNS IP to trigger Connect On Demand */
	if(dns_list->num_servers == 0 && 
	(  nvram_match("wan_proto","pppoe") 
	|| nvram_match("wan_proto","pptp") 
	|| nvram_match("wan_proto","unnumberip") 
	|| nvram_match("wan_proto","l2tp"))
	&& nvram_match("ppp_demand","1"))
	{
                fprintf(fp_w, "nameserver 1.1.1.1\n");
	}

        fclose(fp_w);
        if(dns_list)    free(dns_list);

	eval("touch", "/tmp/hosts");
				
	return 1;
}

/* Example:
 * lan_ipaddr = 192.168.1.1
 * get_dns_ip("lan_ipaddr", 1); produces "168"
 */
int
get_single_ip(char *ipaddr, int which)
{
	int ip[4]={0,0,0,0};
	int ret;

	ret = sscanf(ipaddr,"%d.%d.%d.%d",&ip[0],&ip[1],&ip[2],&ip[3]);

	return ip[which];	
}

char *
get_complete_lan_ip(char *ip)
{
	static char ipaddr[20];

	int i[4]; 

	if(sscanf(nvram_safe_get("lan_ipaddr"),"%d.%d.%d.%d",&i[0],&i[1],&i[2],&i[3]) != 4)
                 return "0.0.0.0";

	snprintf(ipaddr, sizeof(ipaddr), "%d.%d.%d.%s", i[0],i[1],i[2],ip);

	return ipaddr;
}

char *
get_wan_face(void)
{
	static char wanface[IFNAMSIZ];
	
	if(nvram_match("wan_proto", "pptp") || nvram_match("wan_proto", "l2tp") || nvram_match("wan_proto", "pppoe")
	)
		strncpy(wanface, "ppp+", IFNAMSIZ);
	else
		strncpy(wanface, nvram_safe_get("wan_ifname"), IFNAMSIZ);

	return wanface;
}

int
get_ppp_pid(char *file)
{
	char buf[80];
	int pid = -1;
	if(file_to_buf(file, buf, sizeof(buf))){
		char tmp[80], tmp1[80];
		snprintf(tmp, sizeof(tmp), "/var/run/%s.pid", buf);
		file_to_buf(tmp, tmp1, sizeof(tmp1));
		pid = atoi(tmp1);
	}
	return pid;
}
/*
 =====================================================================================
				by tallest 
 =====================================================================================
 */


int 
osl_ifflags(const char *ifname)
{
    int sockfd;
    struct ifreq ifreq;
    int flags = 0;

    if ((sockfd = socket( AF_INET, SOCK_DGRAM, 0 )) < 0) {
	perror("socket");
	return flags;
    }

    strncpy(ifreq.ifr_name, ifname, IFNAMSIZ);
    if (ioctl(sockfd, SIOCGIFFLAGS, &ifreq) < 0) {
	    flags = 0;
    } else {
	    flags = ifreq.ifr_flags;
    }
    close(sockfd);
    return flags;
}

int
check_wan_link(int num)
{
	int wan_link = 0;
	char wan_if[2][20]={"wan_iface","wan_iface_1"};
	
	if(nvram_match("wan_proto", "pptp") 
	|| nvram_match("wan_proto", "l2tp") 
	|| nvram_match("wan_proto", "pppoe") 
	|| nvram_match("wan_proto", "heartbeat")
	){
		FILE *fp;
		char filename[80];
		char *name;

		if(num == 0)
			strcpy(filename, "/tmp/ppp/link");
		if ((fp = fopen(filename, "r"))){
			int pid = -1;
			fclose(fp);
			if(nvram_match("wan_proto", "heartbeat")){
				char buf[20];
				file_to_buf("/tmp/ppp/link", buf, sizeof(buf));
				pid = atoi(buf);
			}
			else
				pid = get_ppp_pid(filename);
			
			name = find_name_by_proc(pid);
			if(!strncmp(name, "pppoecd", 7) ||	// for PPPoE
			   !strncmp(name, "pppd", 4) ||		// for PPTP
			   !strncmp(name, "bpalogin", 8))	// for HeartBeat
				wan_link = 1;     //connect
			else{
				printf("The %s had been died, remove %s\n", nvram_safe_get("wan_proto"), filename);
				wan_link = 0;	// For some reason, the pppoed had been died, by link file still exist.
				unlink(filename);
			}
		}
	}
	else{
		if(nvram_invmatch("wan_ipaddr", "0.0.0.0"))
			wan_link = 1;
	}

	/* Check interface status */
    	if(!(osl_ifflags(nvram_safe_get(wan_if[num])) & IFF_UP))
		wan_link = 0;
				
	return wan_link;	
}

int
get_int_len(int num)
{
	char buf[80];

	snprintf(buf, sizeof(buf), "%d", num);
	
	return strlen(buf);
}

int
file_to_buf(char *path, char *buf, int len)
{
	FILE *fp;

	memset(buf, 0 , len);

	if ((fp = fopen(path, "r"))) {
		fgets(buf, len, fp);
		fclose(fp);
		return 1;
	}

	return 0;
}

int
buf_to_file(char *path, char *buf)
{
	FILE *fp;

	if ((fp = fopen(path, "w"))) {
		fprintf(fp, "%s", buf);
		fclose(fp);
		return 1;
	}

	return 0;
}


#define READ_BUF_SIZE 254
/* from busybox find_pid_by_name.c */
pid_t* find_pid_by_name( char* pidName)
{
    DIR *dir;
    struct dirent *next;
    pid_t* pidList=NULL;
    int i=0;

    dir = opendir("/proc");

    while ((next = readdir(dir)) != NULL) {
        FILE *status;
        char filename[READ_BUF_SIZE];
        char buffer[READ_BUF_SIZE];
        char name[READ_BUF_SIZE];

        /* Must skip ".." since that is outside /proc */
        if (strcmp(next->d_name, "..") == 0)
            continue;

        /* If it isn't a number, we don't want it */
        if (!isdigit(*next->d_name))
            continue;

        sprintf(filename, "/proc/%s/status", next->d_name);
        if (! (status = fopen(filename, "r")) ) {
            continue;
        }
        if (fgets(buffer, READ_BUF_SIZE-1, status) == NULL) {
            fclose(status);
            continue;
        }
        fclose(status);

        /* Buffer should contain a string like "Name:   binary_name" */
        sscanf(buffer, "%*s %s", name);
	//printf("buffer=[%s] name=[%s]\n",buffer,name);
        if (strcmp(name, pidName) == 0) {
            pidList=realloc( pidList, sizeof(pid_t) * (i+2));
            pidList[i++]=strtol(next->d_name, NULL, 0);
        }
    }

    if (pidList)
        pidList[i]=0;
    else {
        pidList=realloc( pidList, sizeof(pid_t));
        pidList[0]=-1;
    }
    return pidList;

}

/* Find first process pid with same name from ps command */
int find_pid_by_ps(char* pidName)
{
        FILE * fp;
        int pid= -1 ;
        char line[254];

        if((fp = popen("ps -ax", "r"))){
                while( fgets(line, sizeof(line), fp) != NULL ) {
                        if(strstr(line, pidName)){
                                sscanf(line, "%d", &pid);
                                printf("%s pid is %d\n", pidName, pid);
                                break;
                        }
                }
                pclose(fp);
        }
	
        return pid;
}

/* Find process name by pid from /proc directory */
char *find_name_by_proc(int pid)
{
        FILE *fp;
        char line[254];
	char filename[80];
	static char name[80];

	snprintf(filename, sizeof(filename), "/proc/%d/status", pid);

	if((fp = fopen(filename, "r"))){
		fgets(line, sizeof(line), fp);
        	/* Buffer should contain a string like "Name:   binary_name" */
		sscanf(line, "%*s %s", name);
        	fclose(fp);
		return name;
        }

        return "";
}

/* Find all process pid with same name from ps command */
int *find_all_pid_by_ps(char* pidName)
{
        FILE * fp;
        int pid= -1 ;
        char line[254];
	int *pidList = NULL;
	int i = 0;

        if((fp = popen("ps -ax", "r"))){
                while( fgets(line, sizeof(line), fp) != NULL ) {
                        if(strstr(line, pidName)){
                                sscanf(line, "%d", &pid);
                                printf("%s pid is %d\n", pidName, pid);
            			pidList = realloc( pidList, sizeof(int) * (i+2));
				pidList[i++] = pid;
                        }
                }
                pclose(fp);
        }
	if (pidList)
		pidList[i]=0;
	else {
		pidList = realloc( pidList, sizeof(int));
		pidList[0] = -1;
	}

        return pidList;
}

void
encode(char *buf, int len)
{
	int i;
	char ch;

	for(i=0 ; i<len ; i++){
		ch = (buf[i] & 0x03) << 6;
	        buf[i] = (buf[i] >> 2);                
	        buf[i] &= 0x3f;
	        buf[i] |= ch;
		buf[i] = ~buf[i];
	}
}

void
decode(char *buf, int len)
{
	int i;
	char ch;
	
	for(i=0 ; i<len ; i++){
		ch = (buf[i] & 0xC0) >> 6;
	        buf[i] = (buf[i] << 2) | ch;
		buf[i] = ~buf[i];
	}
}

/*	v1.41.7 => 014107
 *	v1.2	=> 0102
 */
long
convert_ver(char *ver)
{
        char buf[10];
        int v[3];
	int ret;

        ret = sscanf(ver,"v%d.%d.%d", &v[0], &v[1], &v[2]);

	if(ret == 2){
        	snprintf(buf, sizeof(buf), "%02d%02d", v[0], v[1]);
		return atol(buf);
	}
	else if (ret == 3){
        	snprintf(buf, sizeof(buf), "%02d%02d%02d", v[0], v[1], v[2]);
		return atol(buf);
	}
	else
		return -1;
}

/* To avoid user to download old image that is not support intel flash to new hardware with intel flash.
 */
int
check_flash(void)
{
	// The V2 image can support intel flash completely, so we don't want to check.
	if(check_hw_type() == BCM4712_CHIP)
		return FALSE;
	
	// The V1.X some images cann't support intel flash, so we want to avoid user to downgrade.
	if(nvram_match("skip_amd_check", "1")){
		if(strstr(nvram_safe_get("flash_type"), "Intel") && nvram_invmatch("skip_intel_check", "1"))
			return TRUE;
		else
			return FALSE;
	}
	else	// Cann't downgrade to old firmware version, no matter AMD or Intel flash
		return TRUE;
}

int
check_action(void)
{
	char buf[80] = "";
	
	if(file_to_buf(ACTION_FILE, buf, sizeof(buf))){
		if(!strcmp(buf, "ACT_TFTP_UPGRADE")){
			fprintf(stderr, "Upgrading from tftp now ...\n");
			return ACT_TFTP_UPGRADE;
		}
		else if(!strcmp(buf, "ACT_WEBS_UPGRADE")){
			fprintf(stderr, "Upgrading from web (https) now ...\n");
			return ACT_WEBS_UPGRADE;
		}
		else if(!strcmp(buf, "ACT_WEB_UPGRADE")){
			fprintf(stderr, "Upgrading from web (http) now ...\n");
			return ACT_WEB_UPGRADE;
		}
		else if(!strcmp(buf, "ACT_SW_RESTORE")){
			fprintf(stderr, "Receiving restore command from web ...\n");
			return ACT_SW_RESTORE;
		}
		else if(!strcmp(buf, "ACT_HW_RESTORE")){
			fprintf(stderr, "Receiving restore commond from resetbutton ...\n");
			return ACT_HW_RESTORE;
		}
		else if(!strcmp(buf, "ACT_NVRAM_COMMIT")){
			fprintf(stderr, "Committing nvram now ...\n");
			return ACT_NVRAM_COMMIT;
		}
		else if(!strcmp(buf, "ACT_ERASE_NVRAM")){
			fprintf(stderr, "Erasing nvram now ...\n");
			return ACT_ERASE_NVRAM;
		}
	}
	//fprintf(stderr, "Waiting for upgrading....\n");
	return ACT_IDLE;
}

int
check_now_boot(void)
{
	char *ver = nvram_safe_get("pmon_ver");	

	// for 4712
	// The boot_ver value is lower v2.0 (no included)
	if(!strncmp(ver, "PMON", 4)){
		cprintf("The boot is PMON\n");
		return PMON_BOOT;
	}
	// for 4712
	// The boot_ver value is higher v2.0 (included)
	else if(!strncmp(ver, "CFE", 3)){
		cprintf("The boot is CFE\n");
		return CFE_BOOT;
	}
	else{
		cprintf("The boot is UNKNOWN\n");
		return UNKNOWN_BOOT;
	}
}

void
show_hw_type(int type)
{
	if(type == BCM4702_CHIP)
		cprintf("The chipset is BCM4702\n");
	else if(type == BCM5325E_CHIP)
		cprintf("The chipset is BCM4712L + BCM5325E\n");
	else if(type == BCM4704_BCM5325F_CHIP)
		cprintf("The chipset is BCM4704 + BCM5325F\n");
	else if(type == BCM5352E_CHIP)
		cprintf("The chipset is BCM5352E\n");
	else if(type == BCM4712_CHIP)
		cprintf("The chipset is BCM4712 + ADMtek\n");
	else
		cprintf("The chipset is not defined\n");

}

int
check_hw_type(void)
{
	uint boardflags;	

	boardflags = strtoul(nvram_safe_get("boardflags"), NULL, 0);

	if(nvram_match("boardtype", "bcm94710dev"))
		return BCM4702_CHIP;	
	else if (nvram_match("boardtype", "0x0708") && !(boardflags & BFL_ENETADM))
		return BCM5325E_CHIP;
	else if (nvram_match("boardtype", "0x042f") && !(boardflags & BFL_ENETADM))
		return BCM4704_BCM5325F_CHIP;
	else if (nvram_match("boardtype", "0x0467"))
		return BCM5352E_CHIP;
	else if (nvram_match("boardtype", "0x0101"))
		return BCM4712_CHIP;
	else
		return NO_DEFINE_CHIP;
}

int is_exist(char *filename)
{
	FILE *fp;
	
	if((fp = fopen(filename, "r"))){
		fclose(fp);
		return 1;
	}
	return 0;
}

int
ct_openlog(const char *ident, int option, int facility, char *log_name)
{
	int level = atoi(nvram_safe_get(log_name));

	switch(level){
		case CONSOLE_ONLY:
			break;
	}
	return level;
}


void
ct_syslog(int level, int enable, const char *fmt,...)
{
        char buf[1000];
        va_list args;
	
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args); 

	switch(enable){
		case CONSOLE_ONLY:
			cprintf("[%d] %s\n", getpid(), buf);	// print to console
			break;
	}
}

void
ct_logger(int level, const char *fmt,...)
{
}

void
set_ip_forward(char c)
{
	FILE *fp;

	if( (fp=fopen("/proc/sys/net/ipv4/ip_forward", "r+")) ){
		fputc(c, fp);
		fclose(fp);
	} else{
		perror("/proc/sys/net/ipv4/ip_forward");
	}
}


static char *device_name[] = {
	"eth0",
	"qos0"
};

char *get_device_name(void)
{
	int i;
	
	switch (check_hw_type()){
	case BCM5325E_CHIP:
	case BCM4704_BCM5325F_CHIP:
	case BCM5352E_CHIP:
		i = 0;
		break;
	case BCM4702_CHIP:
	case BCM4712_CHIP:
	default:
		i = 1;
		break;
	}
		
	return device_name[i];
}

char *strncpyz(char *dest, char const *src, size_t size)
{
    if (!size--)
	return dest;
    strncpy(dest, src, size);
    dest[size] = 0; /* Make sure the string is null terminated */
    return dest;
}

static int sockets_open(int domain, int type, int protocol)
{
    int fd = socket(domain, type, protocol);

    if (fd<0)
	cprintf("sockets_open: no usable address was found.\n");
    return fd;
}

int sys_netdev_ioctl(int family, int socket, char *if_name, int cmd,
    struct ifreq *ifr)
{
    int rc, s;

    if ((s = socket) < 0)
    {
	if ((s = sockets_open(family, family==AF_PACKET ? SOCK_PACKET : 
	    SOCK_DGRAM, family==AF_PACKET ? htons(ETH_P_ALL) : 0)) < 0)
	{
	    cprintf("sys_netdev_ioctl: failed\n");
	    return -1;
	}
    }
    strncpyz(ifr->ifr_name, if_name, IFNAMSIZ);
    rc = ioctl(s, cmd, ifr);
    if (socket < 0)
	close(s);
    return rc;
}

int set_register_value(unsigned short port_addr, unsigned short option_content)
{
    struct ifreq ifr;
    struct mii_ioctl_data stats;
	
    stats.phy_id=port_addr;
    stats.val_in=option_content;

    ifr.ifr_data = (void *)&stats;
    
    if (sys_netdev_ioctl(AF_INET, -1, get_device_name(), SIOCSMIIREG, &ifr) < 0)
	return -1;
    
    return 0;
}

unsigned long get_register_value(unsigned short id, unsigned short num)
{
    struct ifreq ifr;
    struct mii_ioctl_data stats;
	
    stats.phy_id = id;
    stats.reg_num = num;
    stats.val_in = 0;
    stats.val_out = 0;

    ifr.ifr_data = (void *)&stats;
    
    sys_netdev_ioctl(AF_INET, -1, get_device_name(), SIOCGMIIREG, &ifr);
    
    return ((stats.val_in << 16) | stats.val_out);
}


struct wl_assoc_mac *
get_wl_assoc_mac(int *c)
{
	FILE *fp;
	struct wl_assoc_mac *wlmac = NULL;
	int count;
	char line[80];
	char list[2][20];
	
	wlmac = (struct wl_assoc_mac *) malloc(sizeof(struct wl_assoc_mac));
        count = *c = 0;
	
	if((fp = popen("wl assoclist", "r"))){
		while( fgets(line, sizeof(line), fp) != NULL ) {
			strcpy(list[0],"");
			strcpy(list[1],"");

			if(sscanf(line,"%s %s", list[0], list[1]) != 2)   // assoclist 00:11:22:33:44:55
				continue;
			if(strcmp(list[0], "assoclist"))
				continue;

			if(count > 0)
				wlmac = realloc(wlmac, sizeof(struct wl_assoc_mac) * (count + 1));
			
			memset(&wlmac[count], 0, sizeof(struct wl_assoc_mac));	
			strncpy(wlmac[count].mac, list[1], sizeof(wlmac[0].mac));
			count ++;
		}
		
		pclose(fp);
		//cprintf("Count of wl assoclist mac is %d\n", count);
		*c = count;
		return wlmac;
	}

	return NULL;
}

struct mtu_lists mtu_list[] = {
#if COUNTRY == JAPAN
        { "pppoe", "576",   "1454"},
#else
        { "pppoe", "576",   "1492"},
#endif

        { "pptp", "576",   "1460"},

        { "l2tp", "576",   "1460"},

        { "dhcp", "576",   "1500"},
        { "static", "576",   "1500"},
        { "heartbeat", "576",   "1500"},
        { "default", "576",   "1500"},	// The value must be at last
};

struct mtu_lists *
get_mtu(char *proto) 
{
	struct mtu_lists *v = NULL;

	for(v = mtu_list ; v < &mtu_list[STRUCT_LEN(mtu_list)] ; v++) {
		if(!strcmp(proto, v->proto))
			return v;
	}
	return v;	// Use default settings
}

void
set_host_domain_name(void)
{
	char buf[254];

	/* Allow you to use gethostname to get Host Name */
	snprintf(buf, sizeof(buf), "echo \"%s\" > /proc/sys/kernel/hostname", nvram_safe_get("wan_hostname"));
	system(buf);

	/* Allow you to use getdomainname to get Domain Name */
	if(nvram_invmatch("wan_domain", ""))
		snprintf(buf, sizeof(buf), "echo \"%s\" > /proc/sys/kernel/domainname", nvram_safe_get("wan_domain"));
	else
		snprintf(buf, sizeof(buf), "echo \"%s\" > /proc/sys/kernel/domainname", nvram_safe_get("wan_get_domain"));
	
	system(buf);
}

int
first_time(void)
{
	struct sysinfo info;

	sysinfo(&info);
	if(info.uptime < 20L)
		return 1;
	return 0;	
}

static int
match_one( const char* pattern, int patternlen, const char* string )
    {
    const char* p;

    for ( p = pattern; p - pattern < patternlen; ++p, ++string )
	{
	if ( *p == '?' && *string != '\0' )
	    continue;
	if ( *p == '*' )
	    {
	    int i, pl;
	    ++p;
	    if ( *p == '*' )
		{
		/* Double-wildcard matches anything. */
		++p;
		i = strlen( string );
		}
	    else
		/* Single-wildcard matches anything but slash. */
		i = strcspn( string, "/" );
	    pl = patternlen - ( p - pattern );
	    for ( ; i >= 0; --i )
		if ( match_one( p, pl, &(string[i]) ) )
		    return 1;
	    return 0;
	    }
	if ( *p != *string )
	    return 0;
	}
    if ( *string == '\0' )
	return 1;
    return 0;
    }


/* Simple shell-style filename matcher.  Only does ? * and **, and multiple
** patterns separated by |.  Returns 1 or 0.
*/
int
regmatch( const char* pattern, const char* string )
    {
    const char* or;

    for (;;)
	{
	or = strchr( pattern, '|' );
	if ( or == (char*) 0 )
	    return match_one( pattern, strlen( pattern ), string );
	if ( match_one( pattern, or - pattern, string ) )
	    return 1;
	pattern = or + 1;
	}
    }

struct ip_lists *
find_dns_ip(char *file, char *name, int *c, int type) 
{
	FILE *fp;
	char dname[254];
	char ip[20];
	char line[254];
	struct ip_lists *ip_list = NULL;
	int count;
	int match;

	count = *c = 0;

	ip_list = (struct ip_lists *) malloc(sizeof(struct ip_lists));

	if( (fp = fopen(file, "r")) ) {
		while( fgets(line, sizeof(line), fp) != NULL ) {
			sscanf(line, "%s %s", dname, ip);
			dprintf("find_dns_ip(): name[%s] ip[%s]\n", dname, ip);
			if((type == USE_REGEX && regmatch(name, dname)) ||
			   (type == FULL_SAME && !strcmp(name, dname)) ||
			   (type == PARTIAL_SAME && strstr(name, dname))) {
				int i;
				match = 0;
				for(i=0 ; i<count ; i++) {
					if(!strcmp(ip_list[i].ip, ip)) {
						dprintf("find_dns_ip(): ip[%s] exist\n", ip);
						match = 1;
						break;
					}
				}

				if(!match) {
					dprintf("ip[%s] no match\n", ip);
					if(count > 0)
						ip_list = realloc(ip_list, sizeof(struct ip_lists) * (count + 1));	
	
					strcpy(ip_list[count].ip, ip);
					count ++;
				}
			}
		}
		fclose(fp);
	}
	*c = count;
	dprintf("count=%d\n", count);
	
	if(!count)	free(ip_list);

	return count ? ip_list : NULL;
}

int
find_dns_ip_name(char *file, char *ip, char *name) 
{
	FILE *fp;
	char _name[254], _ip[20];
	char line[254];

	if( (fp = fopen(file, "r")) ) {
		while( fgets(line, sizeof(line), fp) != NULL ) {
			sscanf(line, "%s %s", _name, _ip);
			dprintf("find_dns_ip_name(): name[%s] ip[%s]\n", _name, _ip);

			if(!strcmp(name, _name) && !strcmp(ip, _ip)) {
				fclose(fp);
				return 1;
			}
		}
		fclose(fp);
	}
	return 0;
}

/* Example:
 * ISDIGIT("", 0); return true;
 * ISDIGIT("", 1); return false;
 * ISDIGIT("123", 1); return true;
 */
int
ISDIGIT(char *value, int flag)
{
	int i, tag = TRUE;

	if(!strcmp(value,"")){
		if (flag) return 0;   // null
		else	return 1;
	}

	for(i=0 ; *(value+i) ; i++){
		if(!isdigit(*(value+i))){
			tag = FALSE;
			break;
		}
	}
	return tag;
}

/* Example:
 * ISASCII("", 0); return true;
 * ISASCII("", 1); return false;
 * ISASCII("abc123", 1); return true;
 */
int
ISASCII(char *value, int flag)
{
	int i, tag = TRUE;

	if(!strcmp(value,"")){
		if (flag) return 0;   // null
		else	return 1;
	}

	for(i=0 ; *(value+i) ; i++){
		if(!isascii(*(value+i))){
			tag = FALSE;
			break;
		}
	}
	return tag;
}

int
check_ipaddr(char *value, int type)
{
	struct in_addr ipaddr;
	int ip[4];
	int ret;

	ret = sscanf(value, "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]);

	if (ret != 4 || !inet_aton(value, &ipaddr)) {
		cprintf("The [%s] is not a legal ip address\n", value);		
		return FALSE;
	}
	else 
		return TRUE;
}

int
check_name(char *value, int max, int type)
{
	cprintf("check_name(): value[%s] max[%d]\n", value, max);

	if (!ISASCII(value,1)){
		cprintf("The [%s] is not a ascii code\n", value);		
		return FALSE;
	}
	if (strlen(value) > max) {
		cprintf("The [%s] length [%d] exceed length limit [%d]\n", value, strlen(value), max);		
		return FALSE;
	}

	return TRUE;
}

int
check_range(char *value, int start, int end)
{
	int n;
	
	cprintf("check_range(): value[%s] start[%d] end[%d]\n", value, start, end);

	n = atoi(value);

	if (!ISDIGIT(value,1)) {
		cprintf("The [%s] is not a digital number\n", value);
		return FALSE;
	}

	if (n < start || n > end) {
		cprintf("The [%s] is not between [%d] and [%d]\n", value, start, end);
	}

	return TRUE;
}

int
check_hwaddr(char *value, int type)
{
	unsigned int hwaddr[6];
	int tag = TRUE;
	int i,count;

	/* Check for bad, multicast, broadcast, or null address */
	for(i=0,count=0 ; *(value+i) ; i++){
		if(*(value+i) == ':'){
			if((i+1)%3 != 0){
				tag = FALSE;
				break;
			}	
			count++;
		}
		else if(isxdigit(*(value+i))) /* one of 0 1 2 3 4 5 6 7 8 9 a b c d e f A B C D E F */
			continue;
		else{
			tag = FALSE;
			break;
		}
	}
	
	if (!tag || i != 17 || count != 5)		/* must have 17's characters and 5's ':' */
		tag = FALSE;	
	else if (sscanf(value, "%x:%x:%x:%x:%x:%x",
		   &hwaddr[0], &hwaddr[1], &hwaddr[2],
		   &hwaddr[3], &hwaddr[4], &hwaddr[5]) != 6 ){
	    //(hwaddr[0] & 1) ||		// the bit 7 is 1 
	    //(hwaddr[0] & hwaddr[1] & hwaddr[2] & hwaddr[3] & hwaddr[4] & hwaddr[5]) == 0xff ){ // FF:FF:FF:FF:FF:FF
	    //(hwaddr[0] | hwaddr[1] | hwaddr[2] | hwaddr[3] | hwaddr[4] | hwaddr[5]) == 0x00){ // 00:00:00:00:00:00
		tag = FALSE;
	}
	else
		tag = TRUE;
	
	return tag;
}

int
check_wpapsk(char *value)
{
	int len = strlen(value);
	char *c;

	if (len == 64) {
		for (c = value; *c; c++) {
			if (!isxdigit((int) *c)) {
				cprintf("character %c is not a hexadecimal digit\n", *c);
				return FALSE;
			}
		}
	} else if (len < 8 || len > 63) {
		cprintf("must be between 8 and 63 ASCII characters or 64 hexadecimal digits\n");
		return FALSE;
	}

	return TRUE;
}

int
check_wepkey(char *value, int type)
{
	char *c;

	switch (strlen(value)) {
	case 10:
	case 26:
		for (c = value; *c; c++) {
			if (!isxdigit(*c)) {
				cprintf("character %c is not a hexadecimal digit\n", *c);
				return FALSE;
			}
		}
		break;
	default:
		cprintf("must be 10 or 26 hexadecimal digits\n");
		return FALSE;
	}

	return TRUE;
}

