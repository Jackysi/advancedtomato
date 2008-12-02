#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <broadcom.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
//#include <netinet/in.h>

//#include <linux/mii.h>
//#include <linux/sockios.h>

//#define DEVICE_NAME	(check_hw_type() == BCM5325E_CHIP) ? "eth0" : "qos0"
//#define SIOCSMIIREG	0x8949		/* Write MII PHY register.	*/

/*static char *device_name[] = {
	"eth0",
	"qos0"
};*/

/*struct mii_ioctl_data {
	unsigned short		phy_id;
	unsigned short		reg_num;
	unsigned short		val_in;
	unsigned short		val_out;
};*/

static char *port_option_name[] = {
 	"port_priority_1",
	"port_flow_control_1",
	//{ "port_frame_type_1",
	"port_rate_limit_1",
	"port_priority_2",
	"port_flow_control_2",
	//{ "port_frame_type_2",
	"port_rate_limit_2",
	"port_priority_3",
	"port_flow_control_3",
	//{ "port_frame_type_3",
	"port_rate_limit_3",
	"port_priority_4",
	"port_flow_control_4",
	//{ "port_frame_type_4",
	"port_rate_limit_4",
	"wan_speed",
	"QoS",
	NULL
};

/*char *get_device_name(void)
{
	int i;
	
	switch (check_hw_type()){
	case BCM5325E_CHIP:
	case BCM4704_BCM5325F_CHIP:
		i = 0;
		break;
	case BCM4702_CHIP:
	case BCM4712_CHIP:
	default:
		i = 1;
		break;
	}
		
	return device_name[i];
}*/

static int set_port_option(unsigned short port_addr, unsigned short option_content)
{
    /*struct ifreq ifr;
    struct mii_ioctl_data stats;
	
    stats.phy_id=port_addr;
    stats.val_in=option_content;

    ifr.ifr_data = (void *)&stats;
    
    if (sys_netdev_ioctl(AF_INET, -1, if_name, SIOCSMIIREG, &ifr) < 0)
	return -1;
    
    return 0;*/
    return set_register_value(port_addr, option_content);
}

int
ej_per_port_option(int eid, webs_t wp, int argc, char_t **argv)
{
	char *arg, priority[]="port_priority_X", flow_control[]="port_flow_control_X", frame_type[]="port_frame_type_X", rate_limit[]="port_rate_limit_X";
	int ret = 0;
	
	if (ejArgs(argc, argv, "%s", &arg) < 1) {
		websError(wp, 400, "Insufficient args\n");
		return -1;
	}
	
	snprintf(priority, sizeof(priority), "port_priority_%s", arg);
	snprintf(flow_control, sizeof(flow_control), "port_flow_control_%s", arg);
	snprintf(frame_type, sizeof(frame_type), "port_frame_type_%s", arg);
	snprintf(rate_limit, sizeof(rate_limit), "port_rate_limit_%s", arg);

	//ret += websWrite(wp, "%s,%s,%s,%s",nvram_get(priority),nvram_get(flow_control),nvram_get(frame_type),nvram_get(rate_limit));
	ret += websWrite(wp, "%s,%s,%s",nvram_safe_get(priority),nvram_safe_get(flow_control),nvram_safe_get(rate_limit));
	
	return ret;
}
	
int get_port_option(char *option_name)
{
	short i, len, unfound_idx = -1;

	len = strlen(option_name);
	
	for (i = 0; port_option_name[i]; i++)
    	{
		if (*option_name && !strncmp(option_name, port_option_name[i], len))
	    		return i;
    	}

	return unfound_idx;
}
	
void
validate_port_qos(webs_t wp, char *value, struct variable *v)
{
	short i;
	
	if(!valid_choice(wp, value, v))
		return;
	
	nvram_set(v->name, value);
    	
	i = get_port_option(v->name);
	
	if (i != -1)
		//set_port_option(DEVICE_NAME, i, atoi(value));
		//set_port_option(get_device_name(), i, atoi(value));
		set_port_option(i, atoi(value));
}

/*unsigned long get_register_value(unsigned short id, unsigned short num)
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
}*/

#if 1 //Added by crazy 20070717 - Fixed issue id 7684, 7693
	/*
	   After clicked the Save Settings button on webpage 
	   'Qos.asp' some times, the DUT will crash when testing 
	   throughput. 
	*/
void
validate_ip_forward(webs_t wp, char *value, struct variable *v)
{
	FILE *fp = NULL;
	
	if(!valid_choice(wp, value, v))
		return;

	cprintf("%s == %s\n", v->name, value);
	
	if(NULL != (fp = fopen("/proc/sys/net/ipv4/ip_forward", "w")))
	{
		fputs(value, fp);
		fclose(fp);
	}
}
#endif
