 /*
 * Remaining issues:
 *   + stats support
 *   + multicast support
 *   + media sense
 *   + half/full duplex
 *   - random MAC addr.
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/ethtool.h>
#include <linux/mii.h>
#include <asm/io.h>

#include <linux/sysctl.h>
#include <cy_conf.h>

#define MODULE_NAME "port_based_qos_mod"
#define DEVICE_NAME "qos"
#define MODULE_VERSION "0.0.1"

extern void ReadDataFromRegister(unsigned short addr, unsigned short *hidata, unsigned short *lodata,int select_count);

#ifdef PERFORMANCE_SUPPORT
static struct ctl_table_header *qos_sysctl_header;
static unsigned long qos[28];

static ctl_table mytable[] = {
         { 2000, "qos", 
	   qos, sizeof(qos), 
	   0644, NULL, 
	   proc_dointvec },
         { 0 }
};

static unsigned short statis_addr_map[7][6] ={
	{0x04, 0x06, 0x08, 0x0a, 0x0b, 0x0c},
	{0x16, 0x18, 0x1a, 0x1c, 0x1d, 0x1e},
	{0x0d, 0x0f, 0x11, 0x13, 0x14, 0x15},
	{0x1f, 0x21, 0x23, 0x25, 0x26, 0x27},
	{0x31, 0x33, 0x35, 0x37, 0x38, 0x39},
	{0x28, 0x2a, 0x2c, 0x2e, 0x2f, 0x30},
	{0x01, 0x01, 0x01, 0x01, 0x01, 0x01}
};

unsigned long get_statistic_from_serial(unsigned short port, unsigned short item)
{
    unsigned short hidata, lodata;
	
    ReadDataFromRegister(statis_addr_map[item][port], &hidata, &lodata, 1); 
	return ((hidata << 16) | lodata);
}
#endif

void ReadDataFromRegister_(unsigned short reg_idx, unsigned short *hidata, unsigned short *lodata)
{
    ReadDataFromRegister(reg_idx, hidata, lodata, 1);
}

#ifdef HW_QOS_SUPPORT
struct port_qos_t{
	int addr;
	int content_mask;
	int *content_set; 
};

void WriteDataToRegister_(unsigned short reg_idx, unsigned short content_idx);
extern void write_eeprom(short,short *,int);

#define BANDWIDTH_1_BIT	2
#define BANDWIDTH_2_BIT	4
#define BANDWIDTH_3_BIT	6
#define BANDWIDTH_4_BIT	7

#define PORT_CONFIG_0	0x1
#define PORT_CONFIG_1	0x3
#define PORT_CONFIG_2	0x5
#define PORT_CONFIG_3	0x7
#define PORT_CONFIG_4	0x8
#define BANDWIDTH_CTL_123	0x31
#define BANDWIDTH_CTL_4	0x32
#define BANDWIDTH_CTL_ENABLE	0x33
#define DISCARD_MODE	0x10
#define TOS_PRIO_MAP	0xf

#define PRIORITY_MASK	0xfc7f
#define PRIORITY_DISABLE_MASK	0xfc7e
#define FLOW_CTL_MASK	0xfffe
#define RATE_LIMIT_MASK_1	0xff8f
#define RATE_LIMIT_MASK_2	0xf8ff
#define RATE_LIMIT_MASK_3	0x8fff
#define RATE_LIMIT_MASK_4	0xfff8
#define BANDWIDTH_CTL_MASK	0xff2b
#define DISCARD_MASK	0x0fff
#define SPEED_MASK	0xfff1

#define BANDWIDTH_ENABLE_1	1 << BANDWIDTH_1_BIT//04 
#define BANDWIDTH_ENABLE_2	1 << BANDWIDTH_2_BIT//10 
#define BANDWIDTH_ENABLE_3	1 << BANDWIDTH_3_BIT//40 
#define BANDWIDTH_ENABLE_4	1 << BANDWIDTH_4_BIT//80 
#define BANDWIDTH_CTL_MASK_1	0xffff^BANDWIDTH_ENABLE_1//0xfffb 
#define BANDWIDTH_CTL_MASK_2	0xffff^BANDWIDTH_ENABLE_2//0xffef 
#define BANDWIDTH_CTL_MASK_3	0xffff^BANDWIDTH_ENABLE_3//0xffbf 
#define BANDWIDTH_CTL_MASK_4	0xffff^BANDWIDTH_ENABLE_4//0xff7f 

/*static int disable_content[] = {0x0};
//static int enable_content[] = {0xd4, 0x0cff};//bit 7,6,4,2; Q1=11(50%),Q0=00(0%)*/
//static int sw_content[] = {0x0,0x0c00};//bit 7,6,4,2; Q1=11(50%),Q0=00(0%)
static int sw_content[] = {0x0,0xc000};//bit 7,6,4,2; Q1=11(50%),Q0=00(0%)
static int port_priority_content[] = {0x080,0x380};//Q0,Q3
//static int port_priority_content[] = {0x300,0x0};//Q1,Q0
static int port_flow_ctl_content[] = {0x0,0x1};
static int port_rate_limit_content_1[] = {0x0,0x00,0x10,0x20,0x30,0x40,0x50,0x60,0x70};
static int port_rate_limit_content_2[] = {0x0,0x000,0x100,0x200,0x300,0x400,0x500,0x600,0x700};
static int port_rate_limit_content_3[] = {0x0,0x0000,0x1000,0x2000,0x3000,0x4000,0x5000,0x6000,0x7000};
static int port_rate_limit_content_4[] = {0x0,0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7};
static int port_rate_limit_enable_1[] = {0x0, BANDWIDTH_ENABLE_1};
static int port_rate_limit_enable_2[] = {0x0, BANDWIDTH_ENABLE_2};
static int port_rate_limit_enable_3[] = {0x0, BANDWIDTH_ENABLE_3};
static int port_rate_limit_enable_4[] = {0x0, BANDWIDTH_ENABLE_4};
//static int wan_speed_content[] = {0xe, 0xc, 0x4, 0x8, 0x0};//auto, 100Full, 100Half, 10Full, 10Half
static int wan_speed_content[] = {0x8, 0x0, 0xc, 0x4, 0xe};//10Full, 10Half, 100Full, 100Half, Auto

static struct port_qos_t port_mii_disable[] = {
	{ BANDWIDTH_CTL_ENABLE, BANDWIDTH_CTL_MASK, sw_content},
	//{ DISCARD_MODE, DISCARD_MASK, sw_content},
	{ PORT_CONFIG_1, PRIORITY_MASK, sw_content},//port_priority_1
	{ PORT_CONFIG_2, PRIORITY_MASK, sw_content},//port_priority_2
	{ PORT_CONFIG_3, PRIORITY_MASK, sw_content},//port_priority_3
	{ PORT_CONFIG_4, PRIORITY_MASK, sw_content},//port_priority_4
	{ PORT_CONFIG_1, FLOW_CTL_MASK, &port_flow_ctl_content[1]},//port_flow_control_1
	{ PORT_CONFIG_2, FLOW_CTL_MASK, &port_flow_ctl_content[1]},//port_flow_control_2
	{ PORT_CONFIG_3, FLOW_CTL_MASK, &port_flow_ctl_content[1]},//port_flow_control_3
	{ PORT_CONFIG_4, FLOW_CTL_MASK, &port_flow_ctl_content[1]},//port_flow_control_4
	{ -1}
};

static struct port_qos_t port_mii_enable[] = {
	//{ BANDWIDTH_CTL_ENABLE, BANDWIDTH_CTL_MASK, enable_content},
	//{ DISCARD_MODE, DISCARD_MASK, sw_content},
	{ -1}
};

struct port_qos_t *port_mii_sw_array[] = {port_mii_disable, port_mii_enable};

/*static struct port_qos_t port_mii_addr[] = {
	{ PORT_CONFIG_1, PRIORITY_MASK, port_priority_content},//port_priority_1
	{ PORT_CONFIG_1, FLOW_CTL_MASK, port_flow_ctl_content},//port_flow_control_1
	//{ "port_frame_type_1", 0x3},
	{ BANDWIDTH_CTL_123, RATE_LIMIT_MASK_14, port_rate_limit_content_14},//port_rate_limit_1
	{ PORT_CONFIG_2, PRIORITY_MASK, port_priority_content},//port_priority_2
	{ PORT_CONFIG_2, FLOW_CTL_MASK, port_flow_ctl_content},//port_flow_control_2
	//{ "port_frame_type_2", 0x5},
	{ BANDWIDTH_CTL_123, RATE_LIMIT_MASK_2, port_rate_limit_content_2},//port_rate_limit_2
	{ PORT_CONFIG_3, PRIORITY_MASK, port_priority_content},//port_priority_3
	{ PORT_CONFIG_3, FLOW_CTL_MASK, port_flow_ctl_content},//port_flow_control_3
	//{ "port_frame_type_3", 0x7},
	{ BANDWIDTH_CTL_123, RATE_LIMIT_MASK_3, port_rate_limit_content_3},//port_rate_limit_3
	//{ "port_priority_4", 0x8, 0x380},
	{ PORT_CONFIG_4, PRIORITY_MASK, port_priority_content},//port_priority_4
	{ PORT_CONFIG_4, FLOW_CTL_MASK, port_flow_ctl_content},//port_flow_control_4
	//{ "port_frame_type_4", 0x8},
	{ BANDWIDTH_CTL_4, RATE_LIMIT_MASK_14, port_rate_limit_content_14},//port_rate_limit_4
	{ -1}
};*/

static struct port_qos_t priority_1[] = {
	{ PORT_CONFIG_1, PRIORITY_MASK, port_priority_content},//port_priority_1
	{ -1}
};
static struct port_qos_t flow_control_1[] = {
	{ PORT_CONFIG_1, FLOW_CTL_MASK, port_flow_ctl_content},//port_flow_control_1
	{ -1}
};
static struct port_qos_t rate_limit_1[] = {
	{ BANDWIDTH_CTL_123, RATE_LIMIT_MASK_1, port_rate_limit_content_1},//port_rate_limit_1
	{ BANDWIDTH_CTL_ENABLE, BANDWIDTH_CTL_MASK_1, port_rate_limit_enable_1},//port_rate_limit_4
	{ -1}
};
static struct port_qos_t priority_2[] = {
	{ PORT_CONFIG_2, PRIORITY_MASK, port_priority_content},//port_priority_2
	{ -1}
};
static struct port_qos_t flow_control_2[] = {
	{ PORT_CONFIG_2, FLOW_CTL_MASK, port_flow_ctl_content},//port_flow_control_2
	{ -1}
};
static struct port_qos_t rate_limit_2[] = {
	{ BANDWIDTH_CTL_123, RATE_LIMIT_MASK_2, port_rate_limit_content_2},//port_rate_limit_2
	{ BANDWIDTH_CTL_ENABLE, BANDWIDTH_CTL_MASK_2, port_rate_limit_enable_2},//port_rate_limit_4
	{ -1}
};
static struct port_qos_t priority_3[] = {
	{ PORT_CONFIG_3, PRIORITY_MASK, port_priority_content},//port_priority_3
	{ -1}
};
static struct port_qos_t flow_control_3[] = {
	{ PORT_CONFIG_3, FLOW_CTL_MASK, port_flow_ctl_content},//port_flow_control_3
	{ -1}
};
static struct port_qos_t rate_limit_3[] = {
	{ BANDWIDTH_CTL_123, RATE_LIMIT_MASK_3, port_rate_limit_content_3},//port_rate_limit_3
	{ BANDWIDTH_CTL_ENABLE, BANDWIDTH_CTL_MASK_3, port_rate_limit_enable_3},//port_rate_limit_4
	{ -1}
};
static struct port_qos_t priority_4[] = {
	{ PORT_CONFIG_4, PRIORITY_MASK, port_priority_content},//port_priority_4
	{ -1}
};
static struct port_qos_t flow_control_4[] = {
	{ PORT_CONFIG_4, FLOW_CTL_MASK, port_flow_ctl_content},//port_flow_control_4
	{ -1}
};
static struct port_qos_t rate_limit_4[] = {
	{ BANDWIDTH_CTL_4, RATE_LIMIT_MASK_4, port_rate_limit_content_4},//port_rate_limit_4
	{ BANDWIDTH_CTL_ENABLE, BANDWIDTH_CTL_MASK_4, port_rate_limit_enable_4},//port_rate_limit_4
	{ -1}
};
static struct port_qos_t wan_speed[] = {
	{ PORT_CONFIG_0, SPEED_MASK, wan_speed_content},
	{ -1}
};

static struct port_qos_t *port_mii_addr[] = {
	priority_1,
	flow_control_1,
	rate_limit_1,
	priority_2,
	flow_control_2,
	rate_limit_2,
	priority_3,
	flow_control_3,
	rate_limit_3,
	priority_4,
	flow_control_4,
	rate_limit_4,
	wan_speed,
	NULL
};

void WriteDataToRegister_(unsigned short reg_idx, unsigned short content_idx)
{
    short RegNumber;
    unsigned short data, hidata=0x0, lodata=0x0;
    int i;
    struct port_qos_t *port_qos = port_mii_addr[reg_idx];
    
    //printk("\nWriteDataToRegister_:reg_idx=%d content_idx=%d\n", reg_idx, content_idx);
    if (!port_qos)
	 port_qos = port_mii_sw_array[content_idx];
	
    for (i=0; port_qos[i].addr != -1; i++)
    {
    	RegNumber = port_qos[i].addr;
 	ReadDataFromRegister(RegNumber, &hidata, &lodata, 0);

        if (!(RegNumber % 2)) /* even port number use lower word */
		hidata = lodata;

        data = (hidata & port_qos[i].content_mask) | (((i > 0) && (content_idx > 1))? port_qos[i].content_set[1] : port_qos[i].content_set[content_idx]);
	
	write_eeprom(RegNumber, &data, 1);
   	ReadDataFromRegister(RegNumber, &hidata, &lodata, 0);
    }
   	ReadDataFromRegister(0xf, &hidata, &lodata, 0);
    	
	/*RegNumber = port_mii_addr[reg_idx].addr;
	if (RegNumber == -1)//Disable or Enable
	{
		struct port_qos_t *port_mii_sw = port_mii_sw_array[content_idx];
	
    		printk("\nWriteDataToRegister_:reg_idx=%d content_idx=%d\n", reg_idx, content_idx);
		for (i=0; port_mii_sw[i].addr != -1; i++)
		{
    			RegNumber = port_mii_sw[i].addr;
				
			ReadDataFromRegister(RegNumber, &hidata, &lodata, 0);
        		
			if (!(RegNumber % 2)) 
				hidata = lodata;
			 
		        data = (hidata & port_mii_sw[i].content_mask) | port_mii_sw[i].content_set[i];
	
			write_eeprom(RegNumber, &data, 1);
    			
   			ReadDataFromRegister(RegNumber, &hidata, &lodata, 0);
			printk("\n============== %s===============\n", (content_idx==0)?"disable":"enable");
		}
	}
	else
	{
    		ReadDataFromRegister(RegNumber, &hidata, &lodata, 0);

        	if (!(RegNumber % 2)) 
			hidata = lodata;

	        data = (hidata & port_mii_addr[reg_idx].content_mask) | port_mii_addr[reg_idx].content_set[content_idx];
	
		write_eeprom(RegNumber, &data, 1);
   		ReadDataFromRegister(RegNumber, &hidata, &lodata, 0);
	}*/
}
#endif

static int dev_do_ioctl(struct net_device *dev, struct ifreq *req, int cmd)
{
    struct mii_ioctl_data *data = (struct mii_ioctl_data *)req->ifr_data;
#ifdef PERFORMANCE_SUPPORT
    int item, port; 
    
    unsigned long status_item;
#endif
    
    switch (cmd)
    {
    case SIOCGMIIPHY:		/* Get address of MII PHY in use. */
    case SIOCDEVPRIVATE:	/* for binary compat, remove in 2.5 */

	/* Fall through to the SIOCGMIIREG, taken from eepro100 and rtl
	 * drivers */
    case SIOCGMIIREG:		/* Read MII PHY register. */
#ifdef PERFORMANCE_SUPPORT
	for (item=0; item<6; item++)
		for (port=1; port<5; port++){
			qos[(item * 4) + (port-1)] = get_statistic_from_serial(port, item);	
		}
	
	status_item = get_statistic_from_serial(0, 6);
	
	qos[24] = (0x1 & (status_item >> 8));
	qos[25] = (0x1 & (status_item >> 16));
	qos[26] = (0x1 & (status_item >> 24));
	qos[27] = (0x1 & (status_item >> 28));
	
	return 0;
#else
   	ReadDataFromRegister_(data->phy_id, &data->val_in, &data->val_out); 
	return 0;
		
#endif

    case SIOCDEVPRIVATE+1:	/* for binary compat, remove in 2.5 */
#ifdef HW_QOS_SUPPORT
    case SIOCSMIIREG:		/* Write MII PHY register. */
	{
	//printk("\n x phy_id=%x\n", data->phy_id);
	//printk("\n x reg_num=%x\n", data->reg_num);
	//printk("\n x val_in=%x\n", data->val_in);
	//printk("\n x val_out=%x\n", data->val_out);
		
   	WriteDataToRegister_(data->phy_id, data->val_in); 
	return 0;
	}
#endif
    case SIOCDEVPRIVATE+2:	/* for binary compat, remove in 2.5 */
    default:
	return -EOPNOTSUPP;
    }
}

static int __devinit qos_eth_probe(struct net_device *dev)
{
    
    SET_MODULE_OWNER(dev);

    ether_setup(dev);

    strcpy(dev->name, DEVICE_NAME "0");

    dev->do_ioctl = dev_do_ioctl;

    return 0;
}

#ifdef HW_QOS_SUPPORT
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
	//{ "port_priority_4", PORT_CONFIG_4, PRIORITY_MASK, port_priority_content},
	"port_flow_control_4",
	//{ "port_frame_type_4",
	"port_rate_limit_4",
	"wan_speed",
	"QoS",
	NULL
};

extern char *nvram_get(const char *name);
extern uint bcm_atoi(char *s);

static int set_port_option(struct net_device *dev, unsigned short port_addr, char *option_content)
{
    struct ifreq ifr;
    struct mii_ioctl_data stats;
	
    stats.phy_id=port_addr;
    stats.val_in=bcm_atoi(option_content);

    ifr.ifr_data = (void *)&stats;
    
    return dev_do_ioctl(dev, &ifr, SIOCSMIIREG);
}


void
restore_default_from_NV(struct net_device *dev)
{
	unsigned short i;
	char *value = NULL;
	
	for (i = 0; port_option_name[i]; i++)
	{
		if((value = nvram_get(port_option_name[i])))
			set_port_option(dev, i, value);
	}
	return;
}
#endif

static struct net_device qos_devices;

/* Module initialization and cleanup */
int init_module(void)
{
    int res;
    struct net_device *dev;

    printk("Initializing " MODULE_NAME " driver " MODULE_VERSION "\n");

	dev = &qos_devices;

	dev->init = qos_eth_probe;

	if ((res = register_netdev(dev)))
	    printk("Failed to register netdev. res = %d\n", res);

#ifdef PERFORMANCE_SUPPORT
    	qos_sysctl_header = register_sysctl_table(mytable, 0);
#endif
#ifdef HW_QOS_SUPPORT
	restore_default_from_NV(dev);
	write_eeprom(TOS_PRIO_MAP, &sw_content[0], 1);/* disable TOS priority map*/
#endif
    return 0;
}

void cleanup_module(void)
{
	struct net_device *dev = &qos_devices;
	if (dev->priv != NULL)
	{
	    unregister_netdev(dev);
	    kfree(dev->priv);
	    dev->priv = NULL;
	}
    	
#ifdef PERFORMANCE_SUPPORT
	unregister_sysctl_table(qos_sysctl_header);
#endif
}

