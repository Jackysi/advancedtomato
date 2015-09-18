/*
 * CDC Ethernet based the networking peripherals of Huawei data card devices
 * This driver is developed based on usbnet.c and cdc_ether.c
 * Copyright (C) 2009 by Franko Fang (Huawei Technologies Co., Ltd.)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will support Huawei data card devices for Linux networking,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */



#include <linux/module.h>
#include <linux/init.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/ethtool.h>
#include <linux/workqueue.h>
#include <linux/mii.h>
#include <linux/usb.h>
#include <linux/sched.h>
#include <linux/ctype.h>
#include <linux/usb/cdc.h>
#include <linux/usbdevice_fs.h>

#include <linux/version.h>
/////////////////////////////////////////////////////////////////////////////////////////////////
#define DRIVER_VERSION "v2.07.00.00"
#define DRIVER_AUTHOR "Franko Fang <huananhu@huawei.com>"
#define DRIVER_DESC "Huawei ether driver for 3G data card ether device"
//////////////////////////////////////////////////////////////////////////////////////////////////////
#define RX_MAX_QUEUE_MEMORY (60 * 1518)
#define    RX_QLEN(dev) ( ((dev)->udev->speed == USB_SPEED_HIGH) ? \
            (RX_MAX_QUEUE_MEMORY / (dev)->rx_urb_size) : 4)
#define    TX_QLEN(dev) (((dev)->udev->speed == USB_SPEED_HIGH) ? \
            (RX_MAX_QUEUE_MEMORY / (dev)->hard_mtu) : 4)

// reawaken network queue this soon after stopping; else watchdog barks
#define TX_TIMEOUT_JIFFIES    (5 * HZ)

// throttle rx/tx briefly after some faults, so khubd might disconnect()
// us (it polls at HZ/4 usually) before we report too many false errors.
#define THROTTLE_JIFFIES    (HZ / 8)

// between wakeups
#define UNLINK_TIMEOUT_MS    3
//////////////////////////////////////////////////////////////////////////////////////////////
// randomly generated ethernet address
static u8    node_id [ETH_ALEN];

static const char driver_name [] = "hw_cdc_net";

/* use ethtool to change the level for any given device */
static int msg_level = -1;
module_param (msg_level, int, 0);
MODULE_PARM_DESC (msg_level, "Override default message level");
//////////////////////////////////////////////////////////////////////////////////////////
#define HW_TLP_MASK_SYNC   0xF800
#define HW_TLP_MASK_LENGTH 0x07FF
#define HW_TLP_BITS_SYNC   0xF800
#pragma pack(push, 1)
struct hw_cdc_tlp
{
    unsigned short pktlength;
    unsigned char payload;
};
#define HW_TLP_HDR_LENGTH sizeof(unsigned short)
#pragma pack(pop)

typedef enum __HW_TLP_BUF_STATE {
    HW_TLP_BUF_STATE_IDLE = 0,
    HW_TLP_BUF_STATE_PARTIAL_FILL,
    HW_TLP_BUF_STATE_PARTIAL_HDR,
    HW_TLP_BUF_STATE_HDR_ONLY,
    HW_TLP_BUF_STATE_ERROR
}HW_TLP_BUF_STATE;

struct hw_cdc_tlp_tmp{
    void *buffer;
    unsigned short pktlength;
    unsigned short bytesneeded;
};
/*max ethernet pkt size 1514*/
#define HW_USB_RECEIVE_BUFFER_SIZE    1600L  
/*for Tin-layer-protocol (TLP)*/
#define HW_USB_MRECEIVE_BUFFER_SIZE   4096L  
/*for TLP*/
#define HW_USB_MRECEIVE_MAX_BUFFER_SIZE (1024 * 16)  

#define HW_JUNGO_BCDDEVICE_VALUE 0x0102
#define BINTERFACESUBCLASS 0x02
///////////////////////////////////////////////////////////////////////////////////////////
#define EVENT_TX_HALT 0
#define EVENT_RX_HALT 1
#define EVENT_RX_MEMORY 2
#define EVENT_STS_SPLIT 3
#define EVENT_LINK_RESET 4


#define NCM_TX_DEFAULT_TIMEOUT_MS 2

static int ncm_prefer_32 = 1;
//module_param(ncm_prefer_32, bool, S_IRUGO);
module_param(ncm_prefer_32, int, S_IRUGO);

static int ncm_prefer_crc = 0;
//module_param(ncm_prefer_crc, bool, S_IRUGO);
module_param(ncm_prefer_crc, int, S_IRUGO);

static unsigned long ncm_tx_timeout = NCM_TX_DEFAULT_TIMEOUT_MS;
module_param(ncm_tx_timeout, ulong, S_IRUGO);

static unsigned int ncm_read_buf_count = 4;
module_param(ncm_read_buf_count, uint, S_IRUGO);

static unsigned short ncm_read_size_in1k = 4;
module_param(ncm_read_size_in1k, short , S_IRUGO);

static int rt_debug = 0;
//module_param(rt_debug, bool, S_IRUGO|S_IWUSR);
module_param(rt_debug, int, S_IRUGO | S_IWUSR);


#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
//#include <linux/unaligned/access_ok.h>
#else
static inline u16 get_unaligned_le16(const void *p)
{
    return le16_to_cpup((__le16 *)p);
}

static inline u32 get_unaligned_le32(const void *p)
{
    return le32_to_cpup((__le32 *)p);
}

static inline void put_unaligned_le16(u16 val, void *p)
{
    *((__le16 *)p) = cpu_to_le16(val);
}

static inline void put_unaligned_le32(u32 val, void *p)
{
    *((__le32 *)p) = cpu_to_le32(val);
}
#endif

/* Add for DTS2011050903736 lxz 20110520 start*/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
#define LINUX_VERSION37_LATER 1
#else
#define LINUX_VERSION37_LATER 0 
#endif
/* Add for DTS2011050903736 lxz 20110520 end*/


/*
  >2.6.36 some syetem not find ncm.h but find cdc.h 
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,35)
#include <linux/usb/ncm.h>
#else
*/
#define USB_CDC_NCM_TYPE        0x1a

/* NCM Functional Descriptor */
/* change usb_cdc_ncm_desc -> usb_cdc_ncm_desc_hw ,prevent cdc.h redefinition 11-05*/
struct usb_cdc_ncm_desc_hw {
    __u8    bLength;
    __u8    bDescriptorType;
    __u8    bDescriptorSubType;
    __le16    bcdNcmVersion;
    __u8    bmNetworkCapabilities;
} __attribute__ ((packed));

#ifdef NCM_NCAP_ETH_FILTER 
#undef NCM_NCAP_ETH_FILTER 
#endif
#ifdef NCM_NCAP_NET_ADDRESS 
#undef NCM_NCAP_NET_ADDRESS 
#endif
#ifdef NCM_NCAP_ENCAP_COMM 
#undef NCM_NCAP_ENCAP_COMM 
#endif
#ifdef NCM_NCAP_MAX_DGRAM 
#undef NCM_NCAP_MAX_DGRAM 
#endif
#ifdef NCM_NCAP_CRC_MODE 
#undef NCM_NCAP_CRC_MODE 
#endif

#define NCM_NCAP_ETH_FILTER    (1 << 0)
#define NCM_NCAP_NET_ADDRESS    (1 << 1)
#define NCM_NCAP_ENCAP_COMM    (1 << 2)
#define NCM_NCAP_MAX_DGRAM    (1 << 3)
#define NCM_NCAP_CRC_MODE    (1 << 4)

#ifdef USB_CDC_GET_NTB_PARAMETERS 
#undef USB_CDC_GET_NTB_PARAMETERS 
#endif
#ifdef USB_CDC_GET_NET_ADDRESS 
#undef USB_CDC_GET_NET_ADDRESS 
#endif
#ifdef USB_CDC_SET_NET_ADDRESS 
#undef USB_CDC_SET_NET_ADDRESS 
#endif
#ifdef USB_CDC_GET_NTB_FORMAT 
#undef USB_CDC_GET_NTB_FORMAT 
#endif
#ifdef USB_CDC_SET_NTB_FORMAT 
#undef USB_CDC_SET_NTB_FORMAT 
#endif
#ifdef USB_CDC_GET_NTB_INPUT_SIZE 
#undef USB_CDC_GET_NTB_INPUT_SIZE 
#endif
#ifdef USB_CDC_SET_NTB_INPUT_SIZE 
#undef USB_CDC_SET_NTB_INPUT_SIZE 
#endif
#ifdef USB_CDC_GET_MAX_DATAGRAM_SIZE 
#undef USB_CDC_GET_MAX_DATAGRAM_SIZE 
#endif
#ifdef USB_CDC_SET_MAX_DATAGRAM_SIZE 
#undef USB_CDC_SET_MAX_DATAGRAM_SIZE 
#endif
#ifdef USB_CDC_GET_CRC_MODE 
#undef USB_CDC_GET_CRC_MODE 
#endif
#ifdef USB_CDC_SET_CRC_MODE 
#undef USB_CDC_SET_CRC_MODE 
#endif

#define USB_CDC_GET_NTB_PARAMETERS        0x80
#define USB_CDC_GET_NET_ADDRESS            0x81
#define USB_CDC_SET_NET_ADDRESS            0x82
#define USB_CDC_GET_NTB_FORMAT            0x83
#define USB_CDC_SET_NTB_FORMAT            0x84
#define USB_CDC_GET_NTB_INPUT_SIZE        0x85
#define USB_CDC_SET_NTB_INPUT_SIZE        0x86
#define USB_CDC_GET_MAX_DATAGRAM_SIZE        0x87
#define USB_CDC_SET_MAX_DATAGRAM_SIZE        0x88
#define USB_CDC_GET_CRC_MODE            0x89
#define USB_CDC_SET_CRC_MODE            0x8a

/*
 * Class Specific structures and constants
 *
 * CDC NCM parameter structure, CDC NCM subclass 6.2.1
 *
 */
struct usb_cdc_ncm_ntb_parameter_hw {
    __le16    wLength;
    __le16    bmNtbFormatSupported;
    __le32    dwNtbInMaxSize;
    __le16    wNdpInDivisor;
    __le16    wNdpInPayloadRemainder;
    __le16    wNdpInAlignment;
    __le16    wPadding1;
    __le32    dwNtbOutMaxSize;
    __le16    wNdpOutDivisor;
    __le16    wNdpOutPayloadRemainder;
    __le16    wNdpOutAlignment;
    __le16    wPadding2;
} __attribute__ ((packed));

/*
 * CDC NCM transfer headers, CDC NCM subclass 3.2
 */
#ifdef NCM_NTH16_SIGN 
#undef NCM_NTH16_SIGN 
#endif
#ifdef NCM_NTH32_SIGN 
#undef NCM_NTH32_SIGN 
#endif

#define NCM_NTH16_SIGN        0x484D434E /* NCMH */
#define NCM_NTH32_SIGN        0x686D636E /* ncmh */

/* change usb_cdc_ncm_nth16 -> usb_cdc_ncm_nth16_hw ,prevent cdc.h redefinition */
struct usb_cdc_ncm_nth16_hw {
    __le32    dwSignature;
    __le16    wHeaderLength;
    __le16    wSequence;
    __le16    wBlockLength;
    __le16    wFpIndex;
} __attribute__ ((packed));

/* change usb_cdc_ncm_nth32 -> usb_cdc_ncm_nth_hw ,prevent cdc.h redefinition */
struct usb_cdc_ncm_nth32_hw {
    __le32    dwSignature;
    __le16    wHeaderLength;
    __le16    wSequence;
    __le32    dwBlockLength;
    __le32    dwFpIndex;
} __attribute__ ((packed));

/*
 * CDC NCM datagram pointers, CDC NCM subclass 3.3
 */
#ifdef NCM_NDP16_CRC_SIGN 
#undef NCM_NDP16_CRC_SIGN 
#endif
#ifdef NCM_NDP16_NOCRC_SIGN 
#undef NCM_NDP16_NOCRC_SIGN 
#endif
#ifdef NCM_NDP32_CRC_SIGN 
#undef NCM_NDP32_CRC_SIGN 
#endif
#ifdef NCM_NDP32_NOCRC_SIGN 
#undef NCM_NDP32_NOCRC_SIGN 
#endif

#define NCM_NDP16_CRC_SIGN    0x314D434E /* NCM1 */
#define NCM_NDP16_NOCRC_SIGN    0x304D434E /* NCM0 */
#define NCM_NDP32_CRC_SIGN    0x316D636E /* ncm1 */
#define NCM_NDP32_NOCRC_SIGN    0x306D636E /* ncm0 */

/* change usb_cdc_ncm_ndp16 -> usb_cdc_ncm_ndp16_hw ,prevent cdc.h redefinition */
struct usb_cdc_ncm_ndp16_hw {
    __le32    dwSignature;
    __le16    wLength;
    __le16    wNextFpIndex;
    __u8    data[0];
} __attribute__ ((packed));

/* change usb_cdc_ncm_ndp32 -> usb_cdc_ncm_ndp32_hw ,prevent cdc.h redefinition */
struct usb_cdc_ncm_ndp32_hw {
    __le32    dwSignature;
    __le16    wLength;
    __le16    wReserved6;
    __le32    dwNextFpIndex;
    __le32    dwReserved12;
    __u8    data[0];
} __attribute__ ((packed));

/*
 * Here are options for NCM Datagram Pointer table (NDP) parser.
 * There are 2 different formats: NDP16 and NDP32 in the spec (ch. 3),
 * in NDP16 offsets and sizes fields are 1 16bit word wide,
 * in NDP32 -- 2 16bit words wide. Also signatures are different.
 * To make the parser code the same, put the differences in the structure,
 * and switch pointers to the structures when the format is changed.
 */

/* change usb_cdc_ncm_ndp32 -> usb_cdc_ncm_ndp32_hw ,prevent redefinition */
struct ndp_parser_opts_hw {
    u32        nth_sign;
    u32        ndp_sign;
    unsigned    nth_size;
    unsigned    ndp_size;
    unsigned    ndplen_align;
    /* sizes in u16 units */
    unsigned    dgram_item_len; /* index or length */
    unsigned    block_length;
    unsigned    fp_index;
    unsigned    reserved1;
    unsigned    reserved2;
    unsigned    next_fp_index;
};

#ifdef INIT_NDP16_OPTS 
#undef INIT_NDP16_OPTS 
#endif
#ifdef INIT_NDP32_OPTS 
#undef INIT_NDP32_OPTS 
#endif

#define INIT_NDP16_OPTS {                    \
        .nth_sign = NCM_NTH16_SIGN,            \
        .ndp_sign = NCM_NDP16_NOCRC_SIGN,        \
        .nth_size = sizeof(struct usb_cdc_ncm_nth16_hw),    \
        .ndp_size = sizeof(struct usb_cdc_ncm_ndp16_hw),    \
        .ndplen_align = 4,                \
        .dgram_item_len = 1,                \
        .block_length = 1,                \
        .fp_index = 1,                    \
        .reserved1 = 0,                    \
        .reserved2 = 0,                    \
        .next_fp_index = 1,                \
    }

#define INIT_NDP32_OPTS {                    \
        .nth_sign = NCM_NTH32_SIGN,            \
        .ndp_sign = NCM_NDP32_NOCRC_SIGN,        \
        .nth_size = sizeof(struct usb_cdc_ncm_nth32_hw),    \
        .ndp_size = sizeof(struct usb_cdc_ncm_ndp32_hw),    \
        .ndplen_align = 8,                \
        .dgram_item_len = 2,                \
        .block_length = 2,                \
        .fp_index = 2,                    \
        .reserved1 = 1,                    \
        .reserved2 = 2,                    \
        .next_fp_index = 2,                \
    }

static inline void put_ncm(__le16 **p, unsigned size, unsigned val)
{
    switch (size) {
    case 1:
        put_unaligned_le16((u16)val, *p);
        break;
    case 2:
        put_unaligned_le32((u32)val, *p);

        break;
    default:
        BUG();
    }

    *p += size;
}

static inline unsigned get_ncm(__le16 **p, unsigned size)
{
    unsigned tmp;

    switch (size) {
    case 1:
        tmp = get_unaligned_le16(*p);
        break;
    case 2:
        tmp = get_unaligned_le32(*p);
        break;
    default:
        BUG();
    }

    *p += size;
    return tmp;
}

#ifdef NCM_CONTROL_TIMEOUT 
#undef NCM_CONTROL_TIMEOUT 
#endif

#define NCM_CONTROL_TIMEOUT        (5 * 1000)
/*#endif*/

/* 'u' must be of unsigned type */
#define IS_POWER2(u) (((u) > 0) && !((u) & ((u) - 1)))

/* 'p' must designate a variable of type * __le16 (in all get/put_ncm_leXX) */
#define get_ncm_le16(p)                \
    ({ __le16 val = get_unaligned_le16(p); p += 1; val; })

#define get_ncm_le32(p)                \
    ({ __le32 val = get_unaligned_le32(p); p += 2; val; })

#define put_ncm_le16(val, p)                \
    ({ put_unaligned_le16((val), p); p += 1; })

#define put_ncm_le32(val, p)                \
    ({ put_unaligned_le32((val), p); p += 2; })

#define NCM_NDP_MIN_ALIGNMENT        4

#ifdef NCM_NTB_MIN_IN_SIZE
#undef NCM_NTB_MIN_IN_SIZE
#endif
#define NCM_NTB_MIN_IN_SIZE        2048

#ifdef NCM_NTB_MIN_OUT_SIZE
#undef NCM_NTB_MIN_OUT_SIZE
#endif

#define NCM_NDP16_ENTRY_LEN        4

/* NTB16 must include: NTB16 header, NDP16 header, datagram pointer entry,
 * terminating (NULL) datagram entry
 */
#define NCM_NTB_MIN_OUT_SIZE        (sizeof(struct usb_cdc_ncm_nth16_hw) \
    + sizeof(struct usb_cdc_ncm_ndp16_hw) + 2 * NCM_NDP16_ENTRY_LEN)

#ifndef max
#define max(_a, _b)     (((_a) > (_b)) ? (_a) : (_b))
#endif

#ifndef min
#define min(_a, _b)     (((_a) < (_b)) ? (_a) : (_b))
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,35)
#define NCM_NTB_HARD_MAX_IN_SIZE ((u32)(max(16,(int)ncm_read_size_in1k) * 1024))
#else
#define NCM_NTB_HARD_MAX_IN_SIZE ((u32)(max(2,(int)ncm_read_size_in1k) * 1024))
#endif

#define RX_QLEN_NCM ncm_read_buf_count
#define TX_QLEN_NCM 4

/* These are actually defined in usbnet.c and we need to redefine these here in
 * order to calculate the size of the SKB pool
 */


static struct ndp_parser_opts_hw ndp16_opts = INIT_NDP16_OPTS;
static struct ndp_parser_opts_hw ndp32_opts = INIT_NDP32_OPTS;

struct ndp_entry {
    struct list_head list;
    unsigned idx;
    unsigned len;
};

struct ntb {
    /* Maximum possible length of this NTB */
    unsigned max_len;
    /* The current offset of the NDP */
    unsigned ndp_off;
    /* The current length of the NDP */
    unsigned ndp_len;
    /* End of the datagrams section */
    unsigned dgrams_end;
    /* Entries list (datagram index/lenght pairs) */
    struct list_head entries;
    /* Number of datagrams in this NTB */
    unsigned ndgrams;
    /* The SKB with the actual NTB data */
    struct sk_buff *skb;
};

#define NTB_LEN(n) ((n)->ndp_off + (n)->ndp_len)
#define NTB_IS_EMPTY(n) ((n)->ndgrams == 0)

struct ncm_ctx {
    struct usb_cdc_ncm_desc_hw *ncm_desc;
    //struct usbnet *unet;
    struct hw_cdc_net *ndev;
    struct usb_interface *control;
    struct usb_interface *data;

#define NTB_FORMAT_SUPPORTED_16BIT 0x0001
#define NTB_FORMAT_SUPPORTED_32BIT 0x0002
    u16 formats;
    u32 rx_max_ntb;
    u32 tx_max_ntb;
    u16 tx_divisor;
    u16 tx_remainder;
    u16 tx_align;

#define NCM_BIT_MODE_16        0
#define NCM_BIT_MODE_32        1
    u8 bit_mode;
#define NCM_CRC_MODE_NO        0
#define NCM_CRC_MODE_YES    1
    u8 crc_mode;

    struct ndp_parser_opts_hw popts;

    struct ntb curr_ntb;
    spinlock_t tx_lock;
    struct sk_buff **skb_pool;
    unsigned skb_pool_size;
    struct timer_list tx_timer;
    /* The maximum amount of jiffies that a datagram can be held (in the
     * current-NTB) before it must be sent on the bus
     */
    unsigned long tx_timeout_jiffies;
#ifdef CONFIG_CDC_ENCAP_COMMAND
    struct cdc_encap *cdc_encap_ctx;
#endif
};


struct hw_cdc_net{
    /* housekeeping */
    struct usb_device    *udev;
    struct usb_interface    *intf;
    const char        *driver_name;
    const char         *driver_desc;
    void            *driver_priv;
    wait_queue_head_t    *wait;
    struct mutex        phy_mutex;
    unsigned char        suspend_count;

    /* i/o info: pipes etc */
    unsigned        in, out;
    struct usb_host_endpoint *status;
    unsigned        maxpacket;
    struct timer_list    delay;

    /* protocol/interface state */
    struct net_device    *net;
    struct net_device_stats    stats;
    int            msg_enable;
    unsigned long        data [5];
    u32            xid;
    u32            hard_mtu;    /* count any extra framing */
    size_t            rx_urb_size;    /* size for rx urbs */
    struct mii_if_info    mii;

    /* various kinds of pending driver work */
    struct sk_buff_head    rxq;
    struct sk_buff_head    txq; 
    struct sk_buff_head    done;
    struct urb        *interrupt;
    struct tasklet_struct    bh;

    struct work_struct    kevent;
    struct delayed_work status_work;//fangxiaozhi added for work
    int            qmi_sync;
    unsigned long        flags;

    /*The state and buffer for the data of TLP*/
    HW_TLP_BUF_STATE hw_tlp_buffer_state;
    struct hw_cdc_tlp_tmp hw_tlp_tmp_buf;
    /*indicate the download tlp feature is activated or not*/
    int hw_tlp_download_is_actived;

    /*Add for ncm */
    int is_ncm;
    struct ncm_ctx *ncm_ctx;
    
};

static inline struct usb_driver *driver_of(struct usb_interface *intf)
{
    return to_usb_driver(intf->dev.driver);
}


/* Drivers that reuse some of the standard USB CDC infrastructure
 * (notably, using multiple interfaces according to the CDC
 * union descriptor) get some helper code.
 */
struct hw_dev_state {
    struct usb_cdc_header_desc    *header;
    struct usb_cdc_union_desc    *u;
    struct usb_cdc_ether_desc    *ether;
    struct usb_interface        *control;
    struct usb_interface        *data;
};


/* we record the state for each of our queued skbs */
enum skb_state {
    illegal = 0,
    tx_start, tx_done,
    rx_start, rx_done, rx_cleanup
};

struct skb_data {    /* skb->cb is one of these */
    struct urb        *urb;
    struct hw_cdc_net        *dev;
    enum skb_state        state;
    size_t            length;
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define devdbg(hw_cdc_net, fmt, arg...) \
    ((void)(rt_debug && printk(KERN_ERR "Hw_cdc_driver######: " fmt "\n" , ## arg)))


#define deverr(hw_cdc_net, fmt, arg...) \
    printk(KERN_ERR "%s: " fmt "\n" , (hw_cdc_net)->net->name , ## arg)
#define devwarn(hw_cdc_net, fmt, arg...) \
    printk(KERN_WARNING "%s: " fmt "\n" , (hw_cdc_net)->net->name , ## arg)

#define devinfo(hw_cdc_net, fmt, arg...) \
    printk(KERN_INFO "%s: " fmt "\n" , (hw_cdc_net)->net->name , ## arg); \


////////////////////////////////////////////////////////////////////////////////
static void hw_cdc_status(struct hw_cdc_net *dev, struct urb *urb);
static inline int hw_get_ethernet_addr(struct hw_cdc_net *dev);
static int hw_cdc_bind(struct hw_cdc_net *dev, struct usb_interface *intf);
void hw_cdc_unbind(struct hw_cdc_net *dev, struct usb_interface *intf);
int cdc_ncm_rx_fixup(struct hw_cdc_net *dev, struct sk_buff *skb);
struct sk_buff * cdc_ncm_tx_fixup(struct hw_cdc_net *dev, struct sk_buff *skb,
    gfp_t mem_flags);
///////////////////////////
int hw_get_endpoints(struct hw_cdc_net *, struct usb_interface *);
void hw_skb_return (struct hw_cdc_net *, struct sk_buff *);
void hw_unlink_rx_urbs(struct hw_cdc_net *);
void hw_defer_kevent (struct hw_cdc_net *, int );
int hw_get_settings (struct net_device *, struct ethtool_cmd *);
int hw_set_settings (struct net_device *, struct ethtool_cmd *);
u32 hw_get_link (struct net_device *);
int hw_nway_reset(struct net_device *);
void hw_get_drvinfo (struct net_device *, struct ethtool_drvinfo *);
u32 hw_get_msglevel (struct net_device *);
void hw_set_msglevel (struct net_device *, u32 );
void hw_disconnect (struct usb_interface *);
int hw_cdc_probe (struct usb_interface *, const struct usb_device_id *);
int hw_resume (struct usb_interface *);
int hw_suspend (struct usb_interface *, pm_message_t );
//////////////////////////

/*Begin : fangxiaozhi added for work*/
static void hw_cdc_check_status_work(struct work_struct *work);
/*{
    struct delayed_work *option_suspend_wq
}*/

/*End : fangxiaozhi added for work*/







/* handles CDC Ethernet and many other network "bulk data" interfaces */
int hw_get_endpoints(struct hw_cdc_net *dev, struct usb_interface *intf)
{
    int                tmp;
    struct usb_host_interface    *alt = NULL;
    struct usb_host_endpoint    *in = NULL, *out = NULL;
    struct usb_host_endpoint    *status = NULL;

    for (tmp = 0; tmp < intf->num_altsetting; tmp++) {
        unsigned    ep;

        //in = out = status = NULL;
        in = NULL;
        out = NULL;
        status = NULL;
        alt = intf->altsetting + tmp;

        /* take the first altsetting with in-bulk + out-bulk;
         * remember any status endpoint, just in case;
         * ignore other endpoints and altsetttings.
         */
        for (ep = 0; ep < alt->desc.bNumEndpoints; ep++) {
            
            struct usb_host_endpoint    *e;
            int                intr = 0;

            e = alt->endpoint + ep;
            switch (e->desc.bmAttributes) {
            case USB_ENDPOINT_XFER_INT:
                if (!usb_endpoint_dir_in(&e->desc)){
                    continue;
                }
                intr = 1;
                /* FALLTHROUGH */
            case USB_ENDPOINT_XFER_BULK:
                break;
            default:
                continue;
            }
            if (usb_endpoint_dir_in(&e->desc)) {
                if (!intr && !in){
                    in = e;
                }else if (intr && !status){
                    status = e;
                }
            } else {
                if (!out){
                    out = e;
                }
            }
        }
        if (in && out){
            break;
        }
    }
    if (!alt || !in || !out){
        return -EINVAL;
    }
    if (alt->desc.bAlternateSetting != 0) {
        tmp = usb_set_interface (dev->udev, alt->desc.bInterfaceNumber,
                alt->desc.bAlternateSetting);
        if (tmp < 0){
            return tmp;
        }
    }

    dev->in = usb_rcvbulkpipe (dev->udev,
            in->desc.bEndpointAddress & USB_ENDPOINT_NUMBER_MASK);
    dev->out = usb_sndbulkpipe (dev->udev,
            out->desc.bEndpointAddress & USB_ENDPOINT_NUMBER_MASK);
    dev->status = status;
    return 0;
}
EXPORT_SYMBOL_GPL(hw_get_endpoints);

static void intr_complete (struct urb *urb);

static int init_status (struct hw_cdc_net *dev, struct usb_interface *intf)
{
    char        *buf = NULL;
    unsigned    pipe = 0;
    unsigned    maxp;
    unsigned    period;


    pipe = usb_rcvintpipe (dev->udev,
            dev->status->desc.bEndpointAddress
                & USB_ENDPOINT_NUMBER_MASK);
    maxp = usb_maxpacket (dev->udev, pipe, 0);

    /* avoid 1 msec chatter:  min 8 msec poll rate */
    period = max ((int) dev->status->desc.bInterval,
        (dev->udev->speed == USB_SPEED_HIGH) ? 7 : 3);

    buf = kmalloc (maxp, GFP_KERNEL);
    if (buf) {
        dev->interrupt = usb_alloc_urb (0, GFP_KERNEL);
        if (!dev->interrupt) {
            kfree (buf);
            return -ENOMEM;
        } else {
            usb_fill_int_urb(dev->interrupt, dev->udev, pipe,
                buf, maxp, intr_complete, dev, period);
            dev_dbg(&intf->dev,
                "status ep%din, %d bytes period %d\n",
                usb_pipeendpoint(pipe), maxp, period);
        }
    }
    return 0;
}


/* Passes this packet up the stack, updating its accounting.
 * Some link protocols batch packets, so their rx_fixup paths
 * can return clones as well as just modify the original skb.
 */
void hw_skb_return (struct hw_cdc_net *dev, struct sk_buff *skb)
{
    int    status;
    u32     sn;

    if(skb->len > 128)
    {
        sn = be32_to_cpu(*(u32 *)(skb->data + 0x26));
        devdbg(dev,"hw_skb_return,len:%d receive sn:%x,  time:%ld-%ld",
               skb->len,sn,current_kernel_time().tv_sec,current_kernel_time().tv_nsec);
    }    
    else
    {
        sn = be32_to_cpu(*(u32 *)(skb->data + 0x2a));
        devdbg(dev,"hw_skb_return,len:%d receive ack sn:%x,  time:%ld-%ld",
               skb->len,sn,current_kernel_time().tv_sec,current_kernel_time().tv_nsec);
    }

    skb->protocol = eth_type_trans (skb, dev->net);
    dev->stats.rx_packets++;
    dev->stats.rx_bytes += skb->len;

    if (netif_msg_rx_status (dev)){
        devdbg (dev, "< rx, len %zu, type 0x%x",
            skb->len + sizeof (struct ethhdr), skb->protocol);
    }
    memset (skb->cb, 0, sizeof (struct skb_data));
    status = netif_rx (skb);
    if (status != NET_RX_SUCCESS && netif_msg_rx_err (dev)){
        devdbg (dev, "netif_rx status %d", status);
    }
}
EXPORT_SYMBOL_GPL(hw_skb_return);

// unlink pending rx/tx; completion handlers do all other cleanup

static int unlink_urbs (struct hw_cdc_net *dev, struct sk_buff_head *q)
{
    unsigned long        flags;
    struct sk_buff        *skb, *skbnext;
    int            count = 0;

    spin_lock_irqsave (&q->lock, flags);
    for (skb = q->next; skb != (struct sk_buff *) q; skb = skbnext) {
        struct skb_data        *entry;
        struct urb        *urb;
        int            retval;

        entry = (struct skb_data *) skb->cb;
        urb = entry->urb;
        skbnext = skb->next;

        // during some PM-driven resume scenarios,
        // these (async) unlinks complete immediately
        retval = usb_unlink_urb (urb);
        if (retval != -EINPROGRESS && retval != 0){
            devdbg (dev, "unlink urb err, %d", retval);
        }
        else
        {
            count++;
        }
    }
    spin_unlock_irqrestore (&q->lock, flags);
    return count;
}


// Flush all pending rx urbs
// minidrivers may need to do this when the MTU changes

void hw_unlink_rx_urbs(struct hw_cdc_net *dev)
{
    if (netif_running(dev->net)) {
        (void) unlink_urbs (dev, &dev->rxq);
        tasklet_schedule(&dev->bh);
    }
}
EXPORT_SYMBOL_GPL(hw_unlink_rx_urbs);


/*-------------------------------------------------------------------------
 *
 * Network Device Driver (peer link to "Host Device", from USB host)
 *
 *-------------------------------------------------------------------------*/

static int hw_change_mtu (struct net_device *net, int new_mtu)
{
    struct hw_cdc_net    *dev = netdev_priv(net);
    int        ll_mtu = new_mtu + net->hard_header_len;
    int        old_hard_mtu = dev->hard_mtu;
    int        old_rx_urb_size = dev->rx_urb_size;


    if (new_mtu <= 0){
        return -EINVAL;
    }
    // no second zero-length packet read wanted after mtu-sized packets
    if ((ll_mtu % dev->maxpacket) == 0){
        return -EDOM;
    }
    net->mtu = new_mtu;

    dev->hard_mtu = net->mtu + net->hard_header_len;
    if (dev->rx_urb_size == old_hard_mtu && !dev->is_ncm) {
        dev->rx_urb_size = dev->hard_mtu;
        if (dev->rx_urb_size > old_rx_urb_size)
        {
            hw_unlink_rx_urbs(dev);
        }
    }

    devdbg(dev,"change mtu :%d, urb_size:%u",new_mtu,(u32)dev->rx_urb_size);

    return 0;
}

/*-------------------------------------------------------------------------*/
//#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 30)
static struct net_device_stats *hw_get_stats (struct net_device *net)
{
    struct hw_cdc_net    *dev = netdev_priv(net);
    return &dev->stats;
}
//#endif
/*-------------------------------------------------------------------------*/

static void tx_defer_bh(struct hw_cdc_net *dev, 
                        struct sk_buff *skb, 
                        struct sk_buff_head *list)
{
    unsigned long        flags;

    spin_lock_irqsave(&list->lock, flags);
    __skb_unlink(skb, list);
    spin_unlock(&list->lock);
    spin_lock(&dev->done.lock);
    __skb_queue_tail(&dev->done, skb);
    if (1 <= dev->done.qlen){
        tasklet_schedule(&dev->bh);
    }
    spin_unlock_irqrestore(&dev->done.lock, flags);
}
////////////////////////////////////////////
static HW_TLP_BUF_STATE submit_skb(struct hw_cdc_net *dev,
                                    unsigned char *data, 
                                    unsigned int len)
{
    struct sk_buff        *skb;
    struct skb_data * entry;
    
    unsigned long flags;
    
    if (len > dev->rx_urb_size){
        devdbg(dev, "The package length is too large\n");
        return HW_TLP_BUF_STATE_ERROR;
    }
    
    if ((skb = alloc_skb (len + NET_IP_ALIGN, GFP_ATOMIC)) == NULL) {
        return HW_TLP_BUF_STATE_ERROR;
    }
    skb_reserve (skb, NET_IP_ALIGN);

    
    entry = (struct skb_data *) skb->cb;
    entry->urb = NULL;
    entry->dev = dev;
    entry->state = rx_done;
    entry->length = skb->len;

    memcpy(skb->data, data, len);
    skb->len = len;

    spin_lock_irqsave(&dev->done.lock, flags);
    __skb_queue_tail(&dev->done, skb);
    if (1 <= dev->done.qlen){
        tasklet_schedule(&dev->bh);
    }
    spin_unlock_irqrestore(&dev->done.lock, flags);
    return HW_TLP_BUF_STATE_IDLE;
}
static void reset_tlp_tmp_buf(struct hw_cdc_net *dev)
{
    dev->hw_tlp_tmp_buf.bytesneeded = 0;
    dev->hw_tlp_tmp_buf.pktlength = 0;
}
static void rx_tlp_parse(struct hw_cdc_net *dev, struct sk_buff *skb)
{
    struct hw_cdc_tlp *tlp = NULL;
    int remain_bytes = (int)skb->len;
    unsigned short pktlen = 0;
    unsigned char *cur_ptr = skb->data;
    unsigned char *payload_ptr = NULL;
    unsigned char *buf_start = skb->data;
    unsigned char *buf_end = buf_start + skb->len;
        unsigned char *ptr = NULL;

    /*decoding the TLP packets into the ether packet*/
    while (remain_bytes > 0){
        switch (dev->hw_tlp_buffer_state){
            case HW_TLP_BUF_STATE_IDLE:
            {
                if (HW_TLP_HDR_LENGTH < remain_bytes ){
                    tlp = (struct hw_cdc_tlp *)cur_ptr;
                    pktlen = (tlp->pktlength & HW_TLP_MASK_LENGTH);
                    payload_ptr = (unsigned char *)&(tlp->payload);

                    //validate the tlp packet header
                    if (HW_TLP_BITS_SYNC != (tlp->pktlength & HW_TLP_MASK_SYNC)){
                        devdbg(dev, "The pktlength is error");                
                        dev->hw_tlp_buffer_state = HW_TLP_BUF_STATE_ERROR;
                        break;
                    }
                    /*The receiced buffer has the whole ether packet */
                    if ( (payload_ptr + pktlen) <= buf_end){
                        /*Get the ether packet from the TLP packet, and put it into the done queue*/
                        submit_skb(dev, payload_ptr, pktlen);
                        cur_ptr = payload_ptr + pktlen;
                        remain_bytes = buf_end - cur_ptr;
                    }else{/*has the part of the ether packet*/
                        if (pktlen > dev->rx_urb_size){
                            devdbg(dev, "The pktlen is invalid");
                            dev->hw_tlp_buffer_state = HW_TLP_BUF_STATE_ERROR;
                            break;
                        }
                        dev->hw_tlp_tmp_buf.bytesneeded = (payload_ptr + pktlen) - buf_end;
                        dev->hw_tlp_tmp_buf.pktlength = buf_end - payload_ptr;
                        memcpy(dev->hw_tlp_tmp_buf.buffer, payload_ptr, 
                               dev->hw_tlp_tmp_buf.pktlength);
                        dev->hw_tlp_buffer_state = HW_TLP_BUF_STATE_PARTIAL_FILL;
                        remain_bytes = 0;
                    }
                }
                else if (HW_TLP_HDR_LENGTH == remain_bytes){
                    memcpy(dev->hw_tlp_tmp_buf.buffer, cur_ptr, remain_bytes);
                    dev->hw_tlp_tmp_buf.bytesneeded = 0;
                    dev->hw_tlp_tmp_buf.pktlength = remain_bytes;
                    dev->hw_tlp_buffer_state = HW_TLP_BUF_STATE_HDR_ONLY;
                    remain_bytes = 0;
                }
                else if (remain_bytes > 0){
                    memcpy(dev->hw_tlp_tmp_buf.buffer, cur_ptr, remain_bytes);
                    dev->hw_tlp_tmp_buf.bytesneeded = HW_TLP_HDR_LENGTH - remain_bytes;
                    dev->hw_tlp_tmp_buf.pktlength = remain_bytes;
                    dev->hw_tlp_buffer_state = HW_TLP_BUF_STATE_PARTIAL_HDR;
                    remain_bytes = 0;
                }
                else{
                    dev->hw_tlp_buffer_state = HW_TLP_BUF_STATE_ERROR;
                }
                break;
            }
            case HW_TLP_BUF_STATE_HDR_ONLY:
            {
                tlp->pktlength = *((unsigned short*)dev->hw_tlp_tmp_buf.buffer);
                pktlen = (tlp->pktlength & HW_TLP_MASK_LENGTH);
                payload_ptr = cur_ptr;
                reset_tlp_tmp_buf(dev);
                /*validate the tlp packet header*/
                if (HW_TLP_BITS_SYNC != (tlp->pktlength & HW_TLP_MASK_SYNC)){
                    devdbg(dev, "The pktlength is error");                
                    dev->hw_tlp_buffer_state = HW_TLP_BUF_STATE_ERROR;
                    break;
                }
                if ( (payload_ptr + pktlen) <= buf_end){
                    submit_skb(dev, payload_ptr, pktlen);
                    cur_ptr = payload_ptr + pktlen;
                    remain_bytes = buf_end - cur_ptr; 
                    dev->hw_tlp_buffer_state = HW_TLP_BUF_STATE_IDLE;
                }else{
                    if (pktlen > dev->rx_urb_size){
                        dev->hw_tlp_buffer_state = HW_TLP_BUF_STATE_ERROR;
                        break;
                    }
                    dev->hw_tlp_tmp_buf.bytesneeded = (payload_ptr + pktlen) - buf_end;
                    dev->hw_tlp_tmp_buf.pktlength = buf_end - payload_ptr;
                    memcpy(dev->hw_tlp_tmp_buf.buffer, payload_ptr, 
                           dev->hw_tlp_tmp_buf.pktlength);
                    dev->hw_tlp_buffer_state = HW_TLP_BUF_STATE_PARTIAL_FILL;
                    remain_bytes = 0;
                }
                break;
            }
            case HW_TLP_BUF_STATE_PARTIAL_HDR:
            {
                memcpy(dev->hw_tlp_tmp_buf.buffer + dev->hw_tlp_tmp_buf.pktlength, 
                       cur_ptr, dev->hw_tlp_tmp_buf.bytesneeded);
                cur_ptr += dev->hw_tlp_tmp_buf.bytesneeded;
                dev->hw_tlp_buffer_state = HW_TLP_BUF_STATE_HDR_ONLY;
                remain_bytes -= dev->hw_tlp_tmp_buf.bytesneeded;
                break;
            }
            case HW_TLP_BUF_STATE_PARTIAL_FILL:
            {
                if (remain_bytes < dev->hw_tlp_tmp_buf.bytesneeded){
                    memcpy(dev->hw_tlp_tmp_buf.buffer + dev->hw_tlp_tmp_buf.pktlength, 
                           cur_ptr, remain_bytes);
                    dev->hw_tlp_tmp_buf.pktlength += remain_bytes;
                    dev->hw_tlp_tmp_buf.bytesneeded -= remain_bytes;
                    dev->hw_tlp_buffer_state = HW_TLP_BUF_STATE_PARTIAL_FILL;
                    cur_ptr += remain_bytes;
                    remain_bytes = 0;
                }else{
                    unsigned short tmplen = dev->hw_tlp_tmp_buf.bytesneeded 
                                          + dev->hw_tlp_tmp_buf.pktlength;
                    if (HW_USB_RECEIVE_BUFFER_SIZE < tmplen){
                        devdbg(dev, "The tlp length is larger than 1600");
                        ptr = (unsigned char *)kmalloc(dev->hw_tlp_tmp_buf.bytesneeded 
                                             + dev->hw_tlp_tmp_buf.pktlength,GFP_KERNEL);
                        if (NULL != ptr){
                            memcpy(ptr, dev->hw_tlp_tmp_buf.buffer, 
                                   dev->hw_tlp_tmp_buf.pktlength);
                            memcpy(ptr + dev->hw_tlp_tmp_buf.pktlength, cur_ptr, 
                                dev->hw_tlp_tmp_buf.bytesneeded);
                            submit_skb(dev, ptr, tmplen);
                            kfree(ptr);
                        }
                        
                    }else{
                        memcpy(dev->hw_tlp_tmp_buf.buffer + dev->hw_tlp_tmp_buf.pktlength, 
                            cur_ptr, dev->hw_tlp_tmp_buf.bytesneeded);
                        submit_skb(dev, dev->hw_tlp_tmp_buf.buffer, tmplen);
                    }
                    remain_bytes -= dev->hw_tlp_tmp_buf.bytesneeded;
                    cur_ptr += dev->hw_tlp_tmp_buf.bytesneeded;
                    dev->hw_tlp_buffer_state = HW_TLP_BUF_STATE_IDLE;
                    reset_tlp_tmp_buf(dev);
                }
                break;
            }
            case HW_TLP_BUF_STATE_ERROR:
            default:
            {
                remain_bytes = 0;
                reset_tlp_tmp_buf(dev);
                dev->hw_tlp_buffer_state = HW_TLP_BUF_STATE_IDLE;
                break;
            }
        }
    }
}

static void rx_defer_bh(struct hw_cdc_net *dev, 
                        struct sk_buff *skb, 
                        struct sk_buff_head *list)
{
    unsigned long        flags;
    spin_lock_irqsave(&list->lock, flags);
    __skb_unlink(skb, list);
    spin_unlock_irqrestore(&list->lock, flags);
    
    /*deal with the download tlp feature*/
    if (1 == dev->hw_tlp_download_is_actived){
        rx_tlp_parse(dev, skb);
        dev_kfree_skb_any(skb);
    }else{
        spin_lock_irqsave(&dev->done.lock, flags);
        __skb_queue_tail(&dev->done, skb);
        if (1 <= dev->done.qlen){
            tasklet_schedule(&dev->bh);
        }
        spin_unlock_irqrestore(&dev->done.lock, flags);
    }
}
////////////////////////

/* some work can't be done in tasklets, so we use keventd
 *
 * NOTE:  annoying asymmetry:  if it's active, schedule_work() fails,
 * but tasklet_schedule() doesn't.  hope the failure is rare.
 */
void hw_defer_kevent (struct hw_cdc_net *dev, int work)
{
    set_bit (work, &dev->flags);
    if (!schedule_work (&dev->kevent)){
        deverr (dev, "kevent %d may have been dropped", work);
    }
    else{
        devdbg (dev, "kevent %d scheduled", work);
    }
}
EXPORT_SYMBOL_GPL(hw_defer_kevent);

/*-------------------------------------------------------------------------*/




static void rx_complete (struct urb *urb);
static void rx_submit (struct hw_cdc_net *dev, struct urb *urb, gfp_t flags)
{
    struct sk_buff        *skb;
    struct skb_data        *entry;
    int            retval = 0;
    unsigned long        lockflags;
    size_t            size = dev->rx_urb_size;

    
    if ((skb = alloc_skb (size + NET_IP_ALIGN, flags)) == NULL) {
        deverr (dev, "no rx skb");
        hw_defer_kevent (dev, EVENT_RX_MEMORY);
        usb_free_urb (urb);
        return;
    }
    skb_reserve (skb, NET_IP_ALIGN);

    entry = (struct skb_data *) skb->cb;
    entry->urb = urb;
    entry->dev = dev;
    entry->state = rx_start;
    entry->length = 0;


    usb_fill_bulk_urb (urb, dev->udev, dev->in,
        skb->data, size, rx_complete, skb);

    spin_lock_irqsave (&dev->rxq.lock, lockflags);


    if (netif_running (dev->net)
            && netif_device_present (dev->net)
            && !test_bit (EVENT_RX_HALT, &dev->flags)) {
        switch (retval = usb_submit_urb (urb, GFP_ATOMIC)) {

        case 0://submit successfully
            __skb_queue_tail (&dev->rxq, skb);
            break;
        case -EPIPE:
            hw_defer_kevent (dev, EVENT_RX_HALT);
            break;
        case -ENOMEM:
            hw_defer_kevent (dev, EVENT_RX_MEMORY);
            break;
        case -ENODEV:
            if (netif_msg_ifdown (dev)){
                devdbg (dev, "device gone");
            }
            netif_device_detach (dev->net);
            break;
        default:
            if (netif_msg_rx_err (dev)){
                devdbg (dev, "rx submit, %d", retval);
            }
            tasklet_schedule (&dev->bh);
            break;
        }
    } else {
        if (netif_msg_ifdown (dev)){
            devdbg (dev, "rx: stopped");
        }
        retval = -ENOLINK;
    }
    spin_unlock_irqrestore (&dev->rxq.lock, lockflags);
    
    devdbg (dev, "usb_submit_urb status:%x, time:%ld-%ld",
            retval,current_kernel_time().tv_sec,current_kernel_time().tv_nsec);

    if (retval) {

        dev_kfree_skb_any (skb);
        usb_free_urb (urb);
    }
}

/*-------------------------------------------------------------------------*/

static inline void rx_process (struct hw_cdc_net *dev, struct sk_buff *skb)
{

    if (dev->is_ncm)
    {   
        if(!cdc_ncm_rx_fixup(dev, skb)){
            goto error;
        }
    }
    if (skb->len){
        hw_skb_return (dev, skb);
    }
    else {
        if (netif_msg_rx_err (dev)){
            devdbg (dev, "drop");
        }
error:
        dev->stats.rx_errors++;
        skb_queue_tail (&dev->done, skb);
    }
}

/*-------------------------------------------------------------------------*/
static void rx_complete (struct urb *urb)
{
    struct sk_buff        *skb = (struct sk_buff *) urb->context;
    struct skb_data        *entry = (struct skb_data *) skb->cb;
    struct hw_cdc_net        *dev = entry->dev;
    int            urb_status = urb->status;


    devdbg (dev, "rx_complete,urb:%p,rx length %d, time %ld-%ld",
            urb, urb->actual_length,current_kernel_time().tv_sec,
            current_kernel_time().tv_nsec);
    skb_put (skb, urb->actual_length);
    entry->state = rx_done;
    entry->urb = NULL;

    switch (urb_status) {
    /* success */
    case 0:
        if (skb->len < dev->net->hard_header_len) {
            entry->state = rx_cleanup;
            dev->stats.rx_errors++;
            dev->stats.rx_length_errors++;
            if (netif_msg_rx_err (dev)){
                devdbg (dev, "rx length %d", skb->len);
            }
        }
        break;

    /* stalls need manual reset. this is rare ... except that
     * when going through USB 2.0 TTs, unplug appears this way.
     * we avoid the highspeed version of the ETIMEOUT/EILSEQ
     * storm, recovering as needed.
     */
    case -EPIPE:
        dev->stats.rx_errors++;
        hw_defer_kevent (dev, EVENT_RX_HALT);
        // FALLTHROUGH

    /* software-driven interface shutdown */
    case -ECONNRESET:        /* async unlink */
    case -ESHUTDOWN:        /* hardware gone */
        if (netif_msg_ifdown (dev)){
            devdbg (dev, "rx shutdown, code %d", urb_status);
        }
        goto block;

    /* we get controller i/o faults during khubd disconnect() delays.
     * throttle down resubmits, to avoid log floods; just temporarily,
     * so we still recover when the fault isn't a khubd delay.
     */
    case -EPROTO:
    case -ETIME:
    case -EILSEQ:
        dev->stats.rx_errors++;
        if (!timer_pending (&dev->delay)) {
            mod_timer (&dev->delay, jiffies + THROTTLE_JIFFIES);
            if (netif_msg_link (dev)){
                devdbg (dev, "rx throttle %d", urb_status);
            }
        }
block:
        entry->state = rx_cleanup;
        entry->urb = urb;
        urb = NULL;
        break;

    /* data overrun ... flush fifo? */
    case -EOVERFLOW:
        dev->stats.rx_over_errors++;
        // FALLTHROUGH

    default:
        entry->state = rx_cleanup;
        dev->stats.rx_errors++;
        if (netif_msg_rx_err (dev)){
            devdbg (dev, "rx status %d", urb_status);
        }
        break;
    }

    rx_defer_bh(dev, skb, &dev->rxq);

    if (urb) {
        if (netif_running (dev->net)
                && !test_bit (EVENT_RX_HALT, &dev->flags)) {
            rx_submit (dev, urb, GFP_ATOMIC);
            return;
        }
        usb_free_urb (urb);
    }
    if (netif_msg_rx_err (dev)){
        devdbg (dev, "no read resubmitted");
    }
}
static void intr_complete (struct urb *urb)
{
    struct hw_cdc_net    *dev = urb->context;
    int        status = urb->status;
    switch (status) {
    /* success */
    case 0:
        hw_cdc_status(dev, urb);
        break;

    /* software-driven interface shutdown */
    case -ENOENT:        /* urb killed */
    case -ESHUTDOWN:    /* hardware gone */
        if (netif_msg_ifdown (dev)){
            devdbg (dev, "intr shutdown, code %d", status);
        }
        return;

    /* NOTE:  not throttling like RX/TX, since this endpoint
     * already polls infrequently
     */
    default:
        devdbg (dev, "intr status %d", status);
        break;
    }

    if (!netif_running (dev->net)){
        return;
    }

    memset(urb->transfer_buffer, 0, urb->transfer_buffer_length);
    status = usb_submit_urb (urb, GFP_ATOMIC);
    if (status != 0 && netif_msg_timer (dev)){
        deverr(dev, "intr resubmit --> %d", status);
    }
}

/*-------------------------------------------------------------------------*/




/*-------------------------------------------------------------------------*/

// precondition: never called in_interrupt

static int hw_stop (struct net_device *net)
{
    struct hw_cdc_net        *dev = netdev_priv(net);
    int            temp;
    DECLARE_WAIT_QUEUE_HEAD_ONSTACK (unlink_wakeup);
    DECLARE_WAITQUEUE (wait, current);

    netif_stop_queue (net);

    if (netif_msg_ifdown (dev)){
        devinfo (dev, "stop stats: rx/tx %ld/%ld, errs %ld/%ld",
            dev->stats.rx_packets, dev->stats.tx_packets,
            dev->stats.rx_errors, dev->stats.tx_errors
            );
    }

    // ensure there are no more active urbs
    add_wait_queue (&unlink_wakeup, &wait);
    dev->wait = &unlink_wakeup;
    temp = unlink_urbs (dev, &dev->txq) + unlink_urbs (dev, &dev->rxq);

    // maybe wait for deletions to finish.
    while (!skb_queue_empty(&dev->rxq)
            && !skb_queue_empty(&dev->txq)
            && !skb_queue_empty(&dev->done)) {
        msleep(UNLINK_TIMEOUT_MS);
        if (netif_msg_ifdown (dev)){
            devdbg (dev, "waited for %d urb completions", temp);
        }
    }
    dev->wait = NULL;
    remove_wait_queue (&unlink_wakeup, &wait);

    /*cleanup the data for TLP*/
    dev->hw_tlp_buffer_state = HW_TLP_BUF_STATE_IDLE;
    if (NULL != dev->hw_tlp_tmp_buf.buffer){
        kfree(dev->hw_tlp_tmp_buf.buffer);
        dev->hw_tlp_tmp_buf.buffer = NULL;
    }
    dev->hw_tlp_tmp_buf.pktlength = 0;
    dev->hw_tlp_tmp_buf.bytesneeded = 0;

    usb_kill_urb(dev->interrupt);

    /* deferred work (task, timer, softirq) must also stop.
     * can't flush_scheduled_work() until we drop rtnl (later),
     * else workers could deadlock; so make workers a NOP.
     */
    dev->flags = 0;
    del_timer_sync (&dev->delay);
    tasklet_kill (&dev->bh);
    usb_autopm_put_interface(dev->intf);

    return 0;
}

/*-------------------------------------------------------------------------*/

// posts reads, and enables write queuing

// precondition: never called in_interrupt

static int hw_open (struct net_device *net)
{
    struct hw_cdc_net        *dev = netdev_priv(net);
    int            retval;
    if ((retval = usb_autopm_get_interface(dev->intf)) < 0) {
        if (netif_msg_ifup (dev)){
            devinfo (dev,
                "resumption fail (%d) hw_cdc_net usb-%s-%s, %s",
                retval,
                dev->udev->bus->bus_name, dev->udev->devpath,
            dev->driver_desc);
        }
        goto done_nopm;
    }    

    /*Initialized the data for TLP*/
    dev->hw_tlp_buffer_state = HW_TLP_BUF_STATE_IDLE;
    dev->hw_tlp_tmp_buf.buffer = kmalloc(HW_USB_RECEIVE_BUFFER_SIZE, GFP_KERNEL);
    if (NULL != dev->hw_tlp_tmp_buf.buffer){
        memset(dev->hw_tlp_tmp_buf.buffer, 0, HW_USB_RECEIVE_BUFFER_SIZE);
    }
    dev->hw_tlp_tmp_buf.pktlength = 0;
    dev->hw_tlp_tmp_buf.bytesneeded = 0;

    
    /* start any status interrupt transfer */
    if (dev->interrupt) {
        retval = usb_submit_urb (dev->interrupt, GFP_KERNEL);
        if (retval < 0) {
            if (netif_msg_ifup (dev)){
                deverr (dev, "intr submit %d", retval);
            }
            goto done;
        }
    }
    
    netif_start_queue (net);

    // delay posting reads until we're fully open
    tasklet_schedule (&dev->bh);
    return retval;
done:
    usb_autopm_put_interface(dev->intf);
done_nopm:
    return retval;
}

/*-------------------------------------------------------------------------*/

/* ethtool methods; minidrivers may need to add some more, but
 * they'll probably want to use this base set.
 */

int hw_get_settings (struct net_device *net, struct ethtool_cmd *cmd)
{
    struct hw_cdc_net *dev = netdev_priv(net);

    if (!dev->mii.mdio_read){
        return -EOPNOTSUPP;
    }

    return mii_ethtool_gset(&dev->mii, cmd);
}
EXPORT_SYMBOL_GPL(hw_get_settings);

int hw_set_settings (struct net_device *net, struct ethtool_cmd *cmd)
{
    struct hw_cdc_net *dev = netdev_priv(net);
    int retval;

    if (!dev->mii.mdio_write){
        return -EOPNOTSUPP;
    }

    retval = mii_ethtool_sset(&dev->mii, cmd);

    return retval;

}
EXPORT_SYMBOL_GPL(hw_set_settings);

u32 hw_get_link (struct net_device *net)
{
    struct hw_cdc_net *dev = netdev_priv(net);

    /* if the device has mii operations, use those */
    if (dev->mii.mdio_read){
        return mii_link_ok(&dev->mii);
    }

    /* Otherwise, say we're up (to avoid breaking scripts) */
    return 1;
}
EXPORT_SYMBOL_GPL(hw_get_link);

int hw_nway_reset(struct net_device *net)
{
    struct hw_cdc_net *dev = netdev_priv(net);

    if (!dev->mii.mdio_write){
        return -EOPNOTSUPP;
    }

    return mii_nway_restart(&dev->mii);
}
EXPORT_SYMBOL_GPL(hw_nway_reset);

void hw_get_drvinfo (struct net_device *net, struct ethtool_drvinfo *info)
{
    struct hw_cdc_net *dev = netdev_priv(net);

    strncpy (info->driver, dev->driver_name, sizeof info->driver);
    strncpy (info->version, DRIVER_VERSION, sizeof info->version);
    strncpy (info->fw_version, dev->driver_desc,
        sizeof info->fw_version);
    usb_make_path (dev->udev, info->bus_info, sizeof info->bus_info);
}
EXPORT_SYMBOL_GPL(hw_get_drvinfo);

u32 hw_get_msglevel (struct net_device *net)
{
    struct hw_cdc_net *dev = netdev_priv(net);

    return dev->msg_enable;
}
EXPORT_SYMBOL_GPL(hw_get_msglevel);

void hw_set_msglevel (struct net_device *net, u32 level)
{
    struct hw_cdc_net *dev = netdev_priv(net);

    dev->msg_enable = level;
}
EXPORT_SYMBOL_GPL(hw_set_msglevel);

/* drivers may override default ethtool_ops in their bind() routine */
static struct ethtool_ops hw_ethtool_ops = {
    .get_settings        = hw_get_settings,
    .set_settings        = hw_set_settings,
    .get_link        = hw_get_link,
    .nway_reset        = hw_nway_reset,
    .get_drvinfo        = hw_get_drvinfo,
    .get_msglevel        = hw_get_msglevel,
    .set_msglevel        = hw_set_msglevel,
};

/*-------------------------------------------------------------------------*/

/* work that cannot be done in interrupt context uses keventd.
 *
 * NOTE:  with 2.5 we could do more of this using completion callbacks,
 * especially now that control transfers can be queued.
 */
static void
kevent (struct work_struct *work)
{
    struct hw_cdc_net        *dev =
        container_of(work, struct hw_cdc_net, kevent);
    int            status;

    /* usb_clear_halt() needs a thread context */
    if (test_bit (EVENT_TX_HALT, &dev->flags)) {
        unlink_urbs (dev, &dev->txq);
        status = usb_clear_halt (dev->udev, dev->out);
        if (status < 0
                && status != -EPIPE
                && status != -ESHUTDOWN) {
            if (netif_msg_tx_err (dev)){
                deverr (dev, "can't clear tx halt, status %d",
                    status);
            }
        } else {
            clear_bit (EVENT_TX_HALT, &dev->flags);
            if (status != -ESHUTDOWN){
                netif_wake_queue (dev->net);
            }
        }
    }
    if (test_bit (EVENT_RX_HALT, &dev->flags)) {
        unlink_urbs (dev, &dev->rxq);
        status = usb_clear_halt (dev->udev, dev->in);
        if (status < 0
                && status != -EPIPE
                && status != -ESHUTDOWN) {
            if (netif_msg_rx_err (dev)){
                deverr (dev, "can't clear rx halt, status %d",
                    status);
            }
        } else {
            clear_bit (EVENT_RX_HALT, &dev->flags);
            tasklet_schedule (&dev->bh);
        }
    }

    /* tasklet could resubmit itself forever if memory is tight */
    if (test_bit (EVENT_RX_MEMORY, &dev->flags)) {
        struct urb    *urb = NULL;

        if (netif_running (dev->net)){
            urb = usb_alloc_urb (0, GFP_KERNEL);
        }else{
            clear_bit (EVENT_RX_MEMORY, &dev->flags);
        }
        if (urb != NULL) {
            clear_bit (EVENT_RX_MEMORY, &dev->flags);
            rx_submit (dev, urb, GFP_KERNEL);
            tasklet_schedule (&dev->bh);
        }
    }

    if (test_bit (EVENT_LINK_RESET, &dev->flags)) {
        clear_bit (EVENT_LINK_RESET, &dev->flags);
    }

    if (dev->flags){
        devdbg (dev, "kevent done, flags = 0x%lx",
            dev->flags);
    }
}

/*-------------------------------------------------------------------------*/

static void tx_complete (struct urb *urb)
{
    struct sk_buff        *skb = (struct sk_buff *) urb->context;
    struct skb_data        *entry = (struct skb_data *) skb->cb;
    struct hw_cdc_net        *dev = entry->dev;

    devdbg(dev,"tx_complete,status:%d,len:%d, *********time:%ld-%ld",
           urb->status,(int)entry->length,
           current_kernel_time().tv_sec,
           current_kernel_time().tv_nsec);

    if (urb->status == 0) {
        dev->stats.tx_packets++;
        dev->stats.tx_bytes += entry->length;
    } else {
        dev->stats.tx_errors++;

        switch (urb->status) {
        case -EPIPE:
            hw_defer_kevent (dev, EVENT_TX_HALT);
            break;

        /* software-driven interface shutdown */
        case -ECONNRESET:        // async unlink
        case -ESHUTDOWN:        // hardware gone
            break;

        // like rx, tx gets controller i/o faults during khubd delays
        // and so it uses the same throttling mechanism.
        case -EPROTO:
        case -ETIME:
        case -EILSEQ:
            if (!timer_pending (&dev->delay)) {
                mod_timer (&dev->delay,
                    jiffies + THROTTLE_JIFFIES);
                if (netif_msg_link (dev)){
                    devdbg (dev, "tx throttle %d",
                            urb->status);
                }
            }
            netif_stop_queue (dev->net);
            break;
        default:
            if (netif_msg_tx_err (dev)){
                devdbg (dev, "tx err %d", entry->urb->status);
            }
            break;
        }
    }

    urb->dev = NULL;
    entry->state = tx_done;
    tx_defer_bh(dev, skb, &dev->txq);
}

/*-------------------------------------------------------------------------*/

static void hw_tx_timeout (struct net_device *net)
{
    struct hw_cdc_net        *dev = netdev_priv(net);

    unlink_urbs (dev, &dev->txq);
    tasklet_schedule (&dev->bh);

    // FIXME: device recovery -- reset?
}

/*-------------------------------------------------------------------------*/

static int hw_start_xmit (struct sk_buff *skb, struct net_device *net)
{
    struct hw_cdc_net        *dev = netdev_priv(net);
    int            length;
    int            retval = NET_XMIT_SUCCESS;
    struct urb        *urb = NULL;
    struct skb_data        *entry;
    unsigned long        flags;

    if (dev->is_ncm ) {
        skb = cdc_ncm_tx_fixup (dev, skb, GFP_ATOMIC);
        if (!skb) {
            if (netif_msg_tx_err (dev)){
                devdbg (dev, "can't tx_fixup skb");
            }
            goto drop;
        }
    }
    
    length = skb->len;

    if (!(urb = usb_alloc_urb (0, GFP_ATOMIC))) {
        if (netif_msg_tx_err (dev)){
            devdbg (dev, "no urb");
        }
        goto drop;
    }

    entry = (struct skb_data *) skb->cb;
    entry->urb = urb;
    entry->dev = dev;
    entry->state = tx_start;
    entry->length = length;

    usb_fill_bulk_urb (urb, dev->udev, dev->out,
            skb->data, skb->len, tx_complete, skb);

    /* don't assume the hardware handles USB_ZERO_PACKET
     * NOTE:  strictly conforming cdc-ether devices should expect
     * the ZLP here, but ignore the one-byte packet.
     */
    if ((length % dev->maxpacket) == 0) {
        urb->transfer_buffer_length++;
        if (skb_tailroom(skb)) {
            skb->data[skb->len] = 0;
            __skb_put(skb, 1);
        }
    }

    devdbg(dev,"hw_start_xmit ,usb_submit_urb,len:%d, time:%ld-%ld",
           skb->len,current_kernel_time().tv_sec,current_kernel_time().tv_nsec);

    spin_lock_irqsave (&dev->txq.lock, flags);

    switch ((retval = usb_submit_urb (urb, GFP_ATOMIC))) {
    case -EPIPE:
        netif_stop_queue (net);
        hw_defer_kevent (dev, EVENT_TX_HALT);
        break;
    default:
        if (netif_msg_tx_err (dev)){
            devdbg (dev, "tx: submit urb err %d", retval);
        }
        break;
    case 0:
        net->trans_start = jiffies;
        __skb_queue_tail (&dev->txq, skb);
        if (dev->txq.qlen >= TX_QLEN (dev)){
            netif_stop_queue (net);
        }
    }
    spin_unlock_irqrestore (&dev->txq.lock, flags);

    if (retval) {
        if (netif_msg_tx_err (dev)){
            devdbg (dev, "drop, code %d", retval);
        }
drop:
        retval = NET_XMIT_SUCCESS;
        dev->stats.tx_dropped++;
        if (skb){
            dev_kfree_skb_any (skb);
        }
        usb_free_urb (urb);
    } else if (netif_msg_tx_queued (dev)) {
        devdbg (dev, "> tx, len %d, type 0x%x",
            length, skb->protocol);
    }
    return retval;
}


/*-------------------------------------------------------------------------*/

// tasklet (work deferred from completions, in_irq) or timer

static void hw_bh (unsigned long param)
{
    struct hw_cdc_net        *dev = (struct hw_cdc_net *) param;
    struct sk_buff        *skb;
    struct skb_data        *entry;

    while ((skb = skb_dequeue (&dev->done))) {
        entry = (struct skb_data *) skb->cb;
        switch (entry->state) {
        case rx_done:
            entry->state = rx_cleanup;
            rx_process (dev, skb);
            continue;
        case tx_done:
        case rx_cleanup:
            usb_free_urb (entry->urb);
            dev_kfree_skb (skb);
            continue;
        default:
            devdbg (dev, "bogus skb state %d", entry->state);
        }
    }

    // waiting for all pending urbs to complete?
    if (dev->wait) {
        if ((dev->txq.qlen + dev->rxq.qlen + dev->done.qlen) == 0) {
            wake_up (dev->wait);
        }

    // or are we maybe short a few urbs?
    } else if (netif_running (dev->net)
            && netif_device_present (dev->net)
            && !timer_pending (&dev->delay)
            && !test_bit (EVENT_RX_HALT, &dev->flags)) {
        int    temp = dev->rxq.qlen;
        int    qlen = dev->is_ncm ? RX_QLEN_NCM : RX_QLEN (dev);


        if (temp < qlen) {
            struct urb    *urb;
            int        i;

            // don't refill the queue all at once
            for (i = 0; i < 10 && dev->rxq.qlen < qlen; i++) {
                urb = usb_alloc_urb (0, GFP_ATOMIC);
                if (urb != NULL){
                    rx_submit (dev, urb, GFP_ATOMIC);
                }
            }
            if (temp != dev->rxq.qlen && netif_msg_link (dev)){
                devdbg (dev, "rxqlen %d --> %d",
                        temp, dev->rxq.qlen);
            }
            if (dev->rxq.qlen < qlen){
                tasklet_schedule (&dev->bh);
            }
        }
        if (dev->txq.qlen < (dev->is_ncm ? TX_QLEN_NCM :TX_QLEN (dev))){
            netif_wake_queue (dev->net);
        }
    }
}


/*-------------------------------------------------------------------------
 *
 * USB Device Driver support
 *
 *-------------------------------------------------------------------------*/

// precondition: never called in_interrupt

void hw_disconnect (struct usb_interface *intf)
{
    struct hw_cdc_net        *dev;
    struct usb_device    *xdev;
    struct net_device    *net;

    dev = usb_get_intfdata(intf);
    usb_set_intfdata(intf, NULL);
    if (!dev){
        return;
    }

    xdev = interface_to_usbdev (intf);

    if (netif_msg_probe (dev)){
        devinfo (dev, "unregister '%s' usb-%s-%s, %s",
            intf->dev.driver->name,
            xdev->bus->bus_name, xdev->devpath,
            dev->driver_desc);
    }

    /*同步取消可能注册的延迟工作进程，如果该工作进程已经在执行了，
    则在这里，需要等待该工作进程执行完成之后，discconect才会继续往下执行。*/
    cancel_delayed_work_sync(&dev->status_work);//Added by fangxz 2010-3-26

    net = dev->net;
    unregister_netdev (net);

    /* we don't hold rtnl here ... */
    flush_scheduled_work ();

    hw_cdc_unbind(dev, intf);

    free_netdev(net);
    usb_put_dev (xdev);
}
EXPORT_SYMBOL_GPL(hw_disconnect);


/*-------------------------------------------------------------------------*/
#if !(LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 30))
static int hw_eth_mac_addr(struct net_device *dev, void *p)
{
    dev->dev_addr[0] = 0x00;
    dev->dev_addr[1] = 0x1e;
    dev->dev_addr[2] = 0x10;
    dev->dev_addr[3] = 0x1f;
    dev->dev_addr[4] = 0x00;
    dev->dev_addr[5] = 0x01;

    return 0;
}
static const struct net_device_ops hw_netdev_ops = {
    .ndo_open = hw_open,
    .ndo_stop = hw_stop,
    .ndo_start_xmit = hw_start_xmit,
    .ndo_tx_timeout = hw_tx_timeout,
    .ndo_change_mtu = hw_change_mtu,
    .ndo_set_mac_address = hw_eth_mac_addr,
    .ndo_validate_addr = eth_validate_addr,
    .ndo_get_stats = hw_get_stats, //用于流量统计
};
#endif

int hw_send_tlp_download_request(struct usb_interface *intf);
// precondition: never called in_interrupt
int hw_check_conn_status(struct usb_interface *intf);


static int is_ncm_interface(struct usb_interface *intf)
{
    u8 bif_class;
    u8 bif_subclass;
    u8 bif_protocol;
    bif_class = intf->cur_altsetting->desc.bInterfaceClass;
    bif_subclass = intf->cur_altsetting->desc.bInterfaceSubClass;
    bif_protocol = intf->cur_altsetting->desc.bInterfaceProtocol;

    if(( bif_class == 0x02 && bif_subclass == 0x0d)  
       ||( bif_class == 0xff && bif_subclass == 0x02 && bif_protocol == 0x16) 
       ||( bif_class == 0xff && bif_subclass == 0x02 && bif_protocol == 0x46)
       ||( bif_class == 0xff && bif_subclass == 0x02 && bif_protocol == 0x76)
      ){
        return 1;
    }
    return 0;
        
}


static int cdc_ncm_config(struct ncm_ctx *ctx)
{
    int                     err;
    struct usb_device             *udev = ctx->ndev->udev;
    u8                    net_caps;
    u8                     control_if;
    unsigned int                tx_pipe;
    unsigned int                rx_pipe;
    struct usb_cdc_ncm_ntb_parameter_hw     *ntb_params;
    u8                    *b;

#define NCM_MAX_CONTROL_MSG sizeof (*ntb_params)

    b = kmalloc(NCM_MAX_CONTROL_MSG, GFP_KERNEL);
    if (unlikely(b == NULL)){
        return -ENOMEM;
    }

    net_caps = ctx->ncm_desc->bmNetworkCapabilities; 
    control_if = ctx->control->cur_altsetting->desc.bInterfaceNumber;
    tx_pipe = usb_sndctrlpipe(udev, 0);
    rx_pipe = usb_rcvctrlpipe(udev, 0);

    err = usb_control_msg(udev, rx_pipe, USB_CDC_GET_NTB_PARAMETERS,
        USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_DIR_IN, 0,
        control_if, b, sizeof(*ntb_params), NCM_CONTROL_TIMEOUT);
    if (err < 0) {
        dev_dbg(&udev->dev, "cannot read NTB params\n");
        goto exit;
    }
    if (err < sizeof(*ntb_params)) {
        dev_dbg(&udev->dev, "the read NTB params block is too short\n");
        err = -EINVAL;
        goto exit;
    }

    ntb_params = (void *)b;
    ctx->formats = le16_to_cpu(ntb_params->bmNtbFormatSupported);
    ctx->rx_max_ntb = le32_to_cpu(ntb_params->dwNtbInMaxSize);
    ctx->tx_max_ntb = le32_to_cpu(ntb_params->dwNtbOutMaxSize);
    ctx->tx_divisor = le16_to_cpu(ntb_params->wNdpOutDivisor);
    ctx->tx_remainder = le16_to_cpu(ntb_params->wNdpOutPayloadRemainder);
    ctx->tx_align = le16_to_cpu(ntb_params->wNdpOutAlignment);

    devdbg(ctx->ndev,"rx_max_ntb:%d,tx_max_ntb:%d,tx_align:%d",
           ctx->rx_max_ntb,ctx->tx_max_ntb,ctx->tx_align);

    if (unlikely(!(ctx->formats & NTB_FORMAT_SUPPORTED_16BIT))) {
        deverr(ctx->ndev, "device does not support 16-bit mode\n");
        err = -EINVAL;
        goto exit;
    }

    if (unlikely(ctx->tx_align < NCM_NDP_MIN_ALIGNMENT)) {
        deverr(ctx->ndev, "wNdpOutAlignment (%u) must be at least "
            "%u\n", ctx->tx_align, NCM_NDP_MIN_ALIGNMENT);
        err = -EINVAL;
        goto exit;
    }

    if (unlikely(!IS_POWER2(ctx->tx_align))) {
        deverr(ctx->ndev, "wNdpOutAlignment (%u) must be a power of "
            "2\n", ctx->tx_align);
        err = -EINVAL;
        goto exit;
    }

    if (unlikely(ctx->rx_max_ntb < NCM_NTB_MIN_IN_SIZE)) {
        deverr(ctx->ndev, "dwNtbInMaxSize (%u) must be at least "
            "%u\n", ctx->rx_max_ntb, NCM_NTB_MIN_IN_SIZE);
        err = -EINVAL;
        goto exit;
    }

    if (ctx->rx_max_ntb > (u32)NCM_NTB_HARD_MAX_IN_SIZE) {
        devdbg(ctx->ndev, "dwNtbInMaxSize (%u) must be at most %u "
            ", setting the device to %u\n",
            ctx->rx_max_ntb, NCM_NTB_HARD_MAX_IN_SIZE,
            NCM_NTB_HARD_MAX_IN_SIZE);
        ctx->rx_max_ntb = NCM_NTB_HARD_MAX_IN_SIZE;
        put_unaligned_le32(ctx->rx_max_ntb, b);
        err = usb_control_msg(udev, tx_pipe,
            USB_CDC_SET_NTB_INPUT_SIZE,
            USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_DIR_OUT,
            0, control_if, b, 4,
            NCM_CONTROL_TIMEOUT);
        if (err < 0) {
            deverr(ctx->ndev, "failed setting NTB input size\n");
            goto exit;
        }
    }
    

    
    if (unlikely(ctx->tx_max_ntb < NCM_NTB_MIN_OUT_SIZE)) {
        deverr(ctx->ndev, "dwNtbOutMaxSize (%u) must be at least "
            "%u\n", ctx->tx_max_ntb, (u32)NCM_NTB_MIN_OUT_SIZE);
        err = -EINVAL;
        goto exit;
    }

    ctx->bit_mode = NCM_BIT_MODE_16;
    if (ncm_prefer_32) {
        if (ctx->formats & NTB_FORMAT_SUPPORTED_32BIT) {
            ctx->bit_mode = NCM_BIT_MODE_32;
        }
        else {
            devinfo(ctx->ndev, "device does not support 32-bit "
                "mode, using 16-bit mode\n");
        }
    }

    /* The spec defines a USB_CDC_SET_NTB_FORMAT as an optional feature.
     * The test for 32-bit support is actually a test if the device
     * implements this request
     */
    if (ctx->formats & NTB_FORMAT_SUPPORTED_32BIT) {
        err = usb_control_msg(udev, tx_pipe,
            USB_CDC_SET_NTB_FORMAT,
            USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_DIR_OUT,
            ctx->bit_mode, control_if, NULL, 0,
            NCM_CONTROL_TIMEOUT);
        if (err < 0) {
            deverr(ctx->ndev, "failed setting bit-mode\n");
            goto exit;
        }
    }

    ctx->crc_mode = NCM_CRC_MODE_NO;
    if (ncm_prefer_crc && (net_caps & NCM_NCAP_CRC_MODE)) {
        ctx->crc_mode = NCM_CRC_MODE_YES;
        err = usb_control_msg(udev, tx_pipe,
            USB_CDC_SET_CRC_MODE,
            USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_DIR_OUT,
            NCM_CRC_MODE_YES, control_if, NULL, 0,
            NCM_CONTROL_TIMEOUT);
        if (err < 0) {
            deverr(ctx->ndev, "failed setting crc-mode\n");
            goto exit;
        }
    }

    switch (ctx->bit_mode)
    {
    case NCM_BIT_MODE_16:
        memcpy(&ctx->popts, &ndp16_opts,
            sizeof (struct ndp_parser_opts_hw));
        if (ctx->crc_mode == NCM_CRC_MODE_YES){
            ctx->popts.ndp_sign = NCM_NDP16_CRC_SIGN;
        }
        break;
    case NCM_BIT_MODE_32:
        memcpy(&ctx->popts, &ndp32_opts,
            sizeof (struct ndp_parser_opts_hw));
        if (ctx->crc_mode == NCM_CRC_MODE_YES){
            ctx->popts.ndp_sign = NCM_NDP32_CRC_SIGN;
        }
        break;
    }

exit:
    kfree(b);
    return err;
#undef NCM_MAX_CONTROL_MSG
}

/* TODO: add crc support */
int cdc_ncm_rx_fixup(struct hw_cdc_net *dev, struct sk_buff *skb)
{
#define NCM_BITS(ctx) (((ctx)->bit_mode == NCM_BIT_MODE_16) ? 16 : 32)
/* Minimal NDP has a header and two entries (each entry has 2 items). */
#define MIN_NDP_LEN(ndp_hdr_size, item_len) ((ndp_hdr_size) + \
    2 * 2 * (sizeof(__le16) * (item_len)))
    struct ncm_ctx        *ctx = dev->ncm_ctx;
    struct usb_device     *udev = dev->udev;
    struct ndp_parser_opts_hw    *popts = &ctx->popts;
    struct sk_buff         *skb2;
    unsigned        skb_len = skb->len;
    __le16            *p = (void *)skb->data;
    __le32            idx;
    __le16            ndp_len;
    unsigned        dgram_item_len = popts->dgram_item_len;
    unsigned         curr_dgram_idx;
    unsigned         curr_dgram_len;
    unsigned        next_dgram_idx;
    unsigned        next_dgram_len;

    u32 rx_len;
    u32 rep_len;
    rx_len = skb->len;
    


    if (unlikely(skb_len < popts->nth_size)) {
        dev_dbg(&udev->dev, "skb len (%u) is shorter than NTH%u len "
            "(%u)\n", skb_len, NCM_BITS(ctx), popts->nth_size);
        goto error;
    }

    if (get_ncm_le32(p) != popts->nth_sign) {
        dev_dbg(&udev->dev, "corrupt NTH%u signature\n", NCM_BITS(ctx));
        goto error;
    }
    
    if (get_ncm_le16(p) != popts->nth_size) {
        dev_dbg(&udev->dev, "wrong NTH%u len\n", NCM_BITS(ctx));
        goto error;
    }

    /* skip sequence num */
    p += 1;

    if (unlikely(get_ncm(&p, popts->block_length) > skb_len)) {
        dev_dbg(&udev->dev, "bogus NTH%u block length\n",
            NCM_BITS(ctx));
        goto error;
    }

    idx = get_ncm(&p, popts->fp_index);
    if (unlikely(idx > skb_len)) {
        dev_dbg(&udev->dev, "NTH%u fp_index (%u) bigger than skb len "
            "(%u)\n", NCM_BITS(ctx), idx, skb_len);
        goto error;
    }

    p = (void *)(skb->data + idx);
    
    if (get_ncm_le32(p) != popts->ndp_sign) {
        dev_dbg(&udev->dev, "corrupt NDP%u signature\n", NCM_BITS(ctx));
        goto error;
    }

    ndp_len = get_ncm_le16(p);
    if (((ndp_len + popts->nth_size) > skb_len)
        || (ndp_len < (MIN_NDP_LEN(popts->ndp_size, dgram_item_len)))) {
        dev_dbg(&udev->dev, "bogus NDP%u len (%u)\n", NCM_BITS(ctx),
            ndp_len);
        goto error;
    }

    p += popts->reserved1;
    /* next_fp_index is defined as reserved in the spec */
    p += popts->next_fp_index;
    p += popts->reserved2;

    curr_dgram_idx = get_ncm(&p, dgram_item_len);
    curr_dgram_len = get_ncm(&p, dgram_item_len);
    next_dgram_idx = get_ncm(&p, dgram_item_len);
    next_dgram_len = get_ncm(&p, dgram_item_len);


    /* Parse all the datagrams in the NTB except for the last one. Pass
     * all the parsed datagrams to the networking stack directly
     */
    rep_len = 0;
    while (next_dgram_idx && next_dgram_len) {
        if (unlikely((curr_dgram_idx + curr_dgram_len) > skb_len)){
            goto error;
        }
        skb2 = skb_clone(skb, GFP_ATOMIC);
        if (unlikely(!skb2)){
            goto error;
        }

        if (unlikely(!skb_pull(skb2, curr_dgram_idx))){
            goto error2;
        }
        skb_trim(skb2, curr_dgram_len);

        rep_len += skb2->len;
        hw_skb_return(dev, skb2);

        curr_dgram_idx = next_dgram_idx;
        curr_dgram_len = next_dgram_len;
        next_dgram_idx = get_ncm(&p, dgram_item_len);
        next_dgram_len = get_ncm(&p, dgram_item_len);
    }

    /* Update 'skb' to represent the last datagram in the NTB and forward
     * it to usbnet which in turn will push it up to the networking stack.
     */
    if (unlikely((curr_dgram_idx + curr_dgram_len) > skb_len)){
        goto error;
    }
    if (unlikely(!skb_pull(skb, curr_dgram_idx))){
        goto error;
    }
    skb_trim(skb, curr_dgram_len);
    rep_len += skb->len;

    return 1;
error2:
    dev_kfree_skb(skb2);
error:
    devdbg(dev,"cdc_ncm_rx_fixup error\n");
    return 0;
#undef NCM_BITS
#undef MIN_NDP_LEN
}

static inline unsigned ndp_dgram_pad(struct ncm_ctx *ctx, unsigned dgram_off)
{
    unsigned rem = dgram_off % ctx->tx_divisor;
    unsigned tmp = ctx->tx_remainder;
    if (rem > ctx->tx_remainder){
        tmp += ctx->tx_divisor;
    }
    return tmp - rem;
}

static inline void ntb_clear(struct ntb *n)
{
    n->ndgrams = 0;
    n->skb = NULL;
    INIT_LIST_HEAD(&n->entries);
}

static inline int ntb_init(struct ncm_ctx *ctx, struct ntb *n, unsigned size)
{
    struct ndp_parser_opts_hw *popts = &ctx->popts;
    unsigned dgrams_end;

    n->max_len = size;
    dgrams_end = popts->nth_size;

    n->ndp_off = ALIGN(dgrams_end, ctx->tx_align);
    n->ndp_len = popts->ndp_size + 2 * 2 * popts->dgram_item_len;
    n->dgrams_end = dgrams_end;

    if (NTB_LEN(n)> n->max_len){
        return -EINVAL;
    }

    ntb_clear(n);
    return 0;
}

static inline int ntb_add_dgram(struct ncm_ctx *ctx, struct ntb *n,
    unsigned dgram_len, u8 *data, gfp_t flags)
{
    struct ndp_parser_opts_hw *popts = &ctx->popts;
    unsigned new_ndp_off;
    unsigned new_ndp_len;
    unsigned new_dgrams_end;
    unsigned dgram_off;
    struct ndp_entry *entry;

    dgram_off = n->dgrams_end + ndp_dgram_pad(ctx, n->dgrams_end);
    new_dgrams_end = dgram_off + dgram_len;

    new_ndp_off = ALIGN(new_dgrams_end, ctx->tx_align);
    new_ndp_len = n->ndp_len + 2 * 2 * popts->dgram_item_len;

    if ((new_ndp_off + new_ndp_len) > n->max_len){
        return -EINVAL;
    }

    /* TODO: optimize to use a kernel lookaside cache (kmem_cache) */
    entry = kmalloc(sizeof(*entry), flags);
    if (unlikely(entry == NULL)){
        return -ENOMEM;
    }

    entry->idx = dgram_off;
    entry->len = dgram_len;
    list_add_tail(&entry->list, &n->entries);

    memcpy(n->skb->data + dgram_off, data, dgram_len);

    n->ndgrams++;

    n->ndp_off = new_ndp_off;
    n->ndp_len = new_ndp_len;
    n->dgrams_end = new_dgrams_end;

    return 0;
}


static inline void ntb_free_dgram_list(struct ntb *n)
{
    struct list_head *p;
    struct list_head *tmp;

    list_for_each_safe(p, tmp, &n->entries) {
        struct ndp_entry *e = list_entry(p, struct ndp_entry, list);
        list_del(p);
        kfree(e);
    }
}

static struct sk_buff *ntb_finalize(struct ncm_ctx *ctx, struct ntb *n)
{
    struct ndp_parser_opts_hw    *popts = &ctx->popts;
    __le16            *p = (void *)n->skb->data;
    struct ndp_entry     *entry;
    struct sk_buff        *skb;

    put_ncm_le32(popts->nth_sign, p);
    put_ncm_le16(popts->nth_size, p);

    /* TODO: add sequence numbers */
    put_ncm_le16(0, p);

    put_ncm(&p, popts->block_length, NTB_LEN(n));
    put_ncm(&p, popts->fp_index, n->ndp_off);

    p = (void *)(n->skb->data + n->ndp_off);
    memset(p, 0, popts->ndp_size);

    put_ncm_le32(popts->ndp_sign, p);
    put_ncm_le16(n->ndp_len, p);

    p += popts->reserved1;
    p += popts->next_fp_index;
    p += popts->reserved2;

    list_for_each_entry(entry, &n->entries, list) {
        put_ncm(&p, popts->dgram_item_len, entry->idx);
        put_ncm(&p, popts->dgram_item_len, entry->len);
    }

    put_ncm(&p, popts->dgram_item_len, 0);
    put_ncm(&p, popts->dgram_item_len, 0);

    ntb_free_dgram_list(n);
    __skb_put(n->skb, NTB_LEN(n));

    skb = n->skb;
    ntb_clear(n);
    
    return skb;
}


static inline struct sk_buff *ncm_get_skb(struct ncm_ctx *ctx)
{
    struct sk_buff         *skb = NULL;
    unsigned         i;

    /* 'skb_shared' will return 0 for an SKB after this SKB was
     * deallocated by usbnet 
     */
    for (i = 0; i < ctx->skb_pool_size && skb_shared(ctx->skb_pool[i]);
        i++);

    if (likely(i < ctx->skb_pool_size)){
        skb = skb_get(ctx->skb_pool[i]);
    }

    if (likely(skb != NULL)){
        __skb_trim(skb, 0);
    }

    return skb;
}


/* Must be run with tx_lock held */
static inline int ncm_init_curr_ntb(struct ncm_ctx *ctx)
{
    struct usb_device     *udev = ctx->ndev->udev;
    int             err;

    err = ntb_init(ctx, &ctx->curr_ntb, ctx->tx_max_ntb);
    if (unlikely(err < 0)) {
        dev_dbg(&udev->dev, "error initializing current-NTB with size "
            "%u\n", ctx->tx_max_ntb);
        return err;
    }

    ctx->curr_ntb.skb = ncm_get_skb(ctx);
    if (unlikely(ctx->curr_ntb.skb == NULL)) {
        dev_dbg(&udev->dev, "failed getting an SKB from the pool\n");
        return -ENOMEM;
    }

    return 0;
}


static inline void ncm_uninit_curr_ntb(struct ncm_ctx *ctx)
{
    dev_kfree_skb_any(ctx->curr_ntb.skb);
    ntb_clear(&ctx->curr_ntb);
}


/* if 'skb' is NULL (timer context), we will finish the current ntb and
 * return it to usbnet
 */
struct sk_buff * cdc_ncm_tx_fixup(struct hw_cdc_net *dev, struct sk_buff *skb,
    gfp_t mem_flags)
{
    struct ncm_ctx         *ctx = dev->ncm_ctx;
    struct ntb            *curr_ntb = &ctx->curr_ntb;
    struct sk_buff         *skb2 = NULL;
    int             err = 0;
    unsigned long        flags;
    unsigned        ndgrams = 0;
    unsigned        is_skb_added = 0;
    unsigned        is_curr_ntb_new = 0;
    u32             sn;

    spin_lock_irqsave(&ctx->tx_lock, flags);

    if (skb == NULL) {
        /* Timer context */
        if (NTB_IS_EMPTY(curr_ntb)) {
            /* we have nothing to send */
            goto exit;
        }
        ndgrams = curr_ntb->ndgrams;
        skb2 = ntb_finalize(ctx, curr_ntb);
        goto exit;
    }

    /* non-timer context */
    if (NTB_IS_EMPTY(curr_ntb)) {
        err = ncm_init_curr_ntb(ctx);
        if (unlikely(err < 0)){
            goto exit;
        }
        is_curr_ntb_new = 1;
    }


    if(skb->len < 128)
    {
        sn = be32_to_cpu(*(u32 *)(skb->data + 0x2a));
        devdbg(dev, "get pc ACK SN:%x  time:%ld-%ld", 
               sn,current_kernel_time().tv_sec,current_kernel_time().tv_nsec);
    }
    else
    {
        sn = be32_to_cpu(*(u32 *)(skb->data + 0x26));        
        devdbg(dev, "get pc PACKETS SN:%x,   time:%ld-%ld", 
               sn,current_kernel_time().tv_sec,current_kernel_time().tv_nsec);
    }

    err = ntb_add_dgram(ctx, curr_ntb, skb->len, skb->data, GFP_ATOMIC);
    switch (err) {
    case 0:
        /* The datagram was successfully added to the current-NTB */
        is_skb_added = 1;
        if(!ctx->tx_timeout_jiffies)
        {
            ndgrams = curr_ntb->ndgrams;
            skb2 = ntb_finalize(ctx, curr_ntb);    
        }
        break;
    case -EINVAL:
        /* not enough space in current-NTB */
        ndgrams = curr_ntb->ndgrams;
        /* finalize the current-NTB */
        skb2 = ntb_finalize(ctx, curr_ntb);
        /* setup a new current-NTB */
        err = ncm_init_curr_ntb(ctx);
        if (unlikely(err < 0)){
            break;
        }

        is_curr_ntb_new = 1;

        err = ntb_add_dgram(ctx, curr_ntb, skb->len, skb->data,
            GFP_ATOMIC);
        if (unlikely(err < 0)) {
            ncm_uninit_curr_ntb(ctx);
            break;
        }

        is_skb_added = 1;
        break;
    default:
        if (is_curr_ntb_new){
            ncm_uninit_curr_ntb(ctx);
        }
        break;
    }

exit:
    if (err){
        devdbg(dev, "tx fixup failed (err %d)\n", err);
    }

    if (skb){
        dev_kfree_skb_any(skb);
    }

    /* When NULL is returned, usbnet will increment the drop count of the
     * net device. If 'skb' was successfully added to the current-NTB,
     * decrement the drop-count ahead
     */
    if (skb2 == NULL && (is_skb_added || skb == NULL))
    {
        if(is_skb_added){
            dev->stats.tx_dropped--;
        }
    }
    /* If a finished NTB is returned to usbnet, it will add 1 to packet
     * count. All other packets that we previously 'dropped' by usbnet must
     * be compensated
     */
    if (skb2 != NULL){
        dev->stats.tx_packets += ndgrams - 1;
    }

    /* reschedule the timer if successfully added a first datagram to a
     * newly allocated current-NTB
     */
    if (is_curr_ntb_new && is_skb_added && ctx->tx_timeout_jiffies){
        mod_timer(&ctx->tx_timer, jiffies + ctx->tx_timeout_jiffies);
    }

    spin_unlock_irqrestore(&ctx->tx_lock, flags);

    return skb2;
}

static void ncm_tx_timer_cb(unsigned long param)
{
    struct ncm_ctx *ctx = (void *)param;
    if (!netif_queue_stopped(ctx->ndev->net)){
        hw_start_xmit(NULL, ctx->ndev->net);
    }

}


int
hw_cdc_probe (struct usb_interface *udev, const struct usb_device_id *prod)
{
    struct hw_cdc_net            *dev;
    struct net_device        *net;
    struct usb_host_interface    *interface;
    struct usb_device        *xdev;
    int                status;
    const char            *name;
//    DECLARE_MAC_BUF(mac);

    name = udev->dev.driver->name;
    xdev = interface_to_usbdev (udev);
    interface = udev->cur_altsetting;

    usb_get_dev (xdev);

    status = -ENOMEM;

    // set up our own records
    net = alloc_etherdev(sizeof(*dev));
    if (!net) {
        dbg ("can't kmalloc dev");
        goto out;
    }

    dev = netdev_priv(net);
    dev->udev = xdev;
    dev->intf = udev;
/* Add for DTS2011050903736 lxz 20110520 start*/
/* linux kernel > 2.6.37: PowerManager needs disable_depth ==0 */
#ifdef  CONFIG_PM_RUNTIME
        if(LINUX_VERSION37_LATER)
        {
          dev->intf->dev.power.disable_depth = 0;
        }
#endif
/* Add for DTS2011050903736 lxz 20110520 end*/

    dev->driver_name = name;
    dev->driver_desc = "Huawei Ethernet Device";
    dev->msg_enable = netif_msg_init (msg_level, NETIF_MSG_DRV
                | NETIF_MSG_PROBE | NETIF_MSG_LINK);
    skb_queue_head_init (&dev->rxq);
    skb_queue_head_init (&dev->txq);
    skb_queue_head_init (&dev->done);
    dev->bh.func = hw_bh;
    dev->bh.data = (unsigned long) dev;
    INIT_WORK (&dev->kevent, kevent);
    dev->delay.function = hw_bh;
    dev->delay.data = (unsigned long) dev;
    init_timer (&dev->delay);
    mutex_init (&dev->phy_mutex);

    dev->net = net;
    //strcpy (net->name, "eth%d");
    memcpy (net->dev_addr, node_id, sizeof node_id);

    /* rx and tx sides can use different message sizes;
     * bind() should set rx_urb_size in that case.
     */
    dev->hard_mtu = net->mtu + net->hard_header_len;

#if !(LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 30))
    net->netdev_ops = &hw_netdev_ops;
#else
    net->change_mtu = hw_change_mtu;
    net->get_stats = hw_get_stats;
    net->hard_start_xmit = hw_start_xmit;
    net->open = hw_open;
    net->stop = hw_stop;
    net->tx_timeout = hw_tx_timeout;
#endif
    net->watchdog_timeo = TX_TIMEOUT_JIFFIES;
    net->ethtool_ops = &hw_ethtool_ops;

    
    status = hw_cdc_bind (dev, udev);
    if (status < 0){
        goto out1;
    }

    
    strcpy (net->name, "eth%d");
    

    /* maybe the remote can't receive an Ethernet MTU */
    if (net->mtu > (dev->hard_mtu - net->hard_header_len)){
        net->mtu = dev->hard_mtu - net->hard_header_len;
    }

    if (status >= 0 && dev->status){
        status = init_status (dev, udev);
    }
    if (status < 0){
        goto out3;
    }
    
    if (dev->is_ncm){
           dev->rx_urb_size = dev->ncm_ctx->rx_max_ntb;
    }else if (!dev->rx_urb_size){
        dev->rx_urb_size = dev->hard_mtu;
    }

    dev->maxpacket = usb_maxpacket (dev->udev, dev->out, 1);

    SET_NETDEV_DEV(net, &udev->dev);
    status = register_netdev (net);
    if (status){
        goto out3;
    }

    if (netif_msg_probe (dev)){
        devinfo (dev, "register '%s', %s",
            dev->driver_name,
            dev->driver_desc);
    }

    // ok, it's ready to go.
    usb_set_intfdata (udev, dev);

    /*activate the download tlp feature*/
    if (0 < hw_send_tlp_download_request(udev)){
        devdbg(dev, "%s: The tlp is activated", __FUNCTION__);
        dev->hw_tlp_download_is_actived = 1;//activated successfully
    }else{
        dev->hw_tlp_download_is_actived = 0;//activated failed
    }
    
    netif_device_attach (net);

    //kernel_thread(hw_check_conn_status, (void *)net, 0);
    
    /*set the carrier off as default*/
    netif_carrier_off(net);
    if (HW_JUNGO_BCDDEVICE_VALUE != dev->udev->descriptor.bcdDevice 
     && BINTERFACESUBCLASS != udev->cur_altsetting->desc.bInterfaceSubClass) {
        dev->qmi_sync = 0;
        INIT_DELAYED_WORK(&dev->status_work, hw_cdc_check_status_work);
            schedule_delayed_work(&dev->status_work, 10*HZ);
    }
    //hw_check_conn_status(udev);
    //
        
    return 0;

out3:
    hw_cdc_unbind (dev, udev);
out1:
    free_netdev(net);
out:
    usb_put_dev(xdev);
    return status;
}
EXPORT_SYMBOL_GPL(hw_cdc_probe);

/*-------------------------------------------------------------------------*/

/*
 * suspend the whole driver as soon as the first interface is suspended
 * resume only when the last interface is resumed
 */

int hw_suspend (struct usb_interface *intf, pm_message_t message)
{
    struct hw_cdc_net        *dev = usb_get_intfdata(intf);

    if (!dev->suspend_count++) {
        /*
         * accelerate emptying of the rx and queues, to avoid
         * having everything error out.
         */
        netif_device_detach (dev->net);
        (void) unlink_urbs (dev, &dev->rxq);
        (void) unlink_urbs (dev, &dev->txq);
        /*
         * reattach so runtime management can use and
         * wake the device
         */
        netif_device_attach (dev->net);
    }
    return 0;
}
EXPORT_SYMBOL_GPL(hw_suspend);

int hw_resume (struct usb_interface *intf)
{
    struct hw_cdc_net        *dev = usb_get_intfdata(intf);

    if (!--dev->suspend_count){
        tasklet_schedule (&dev->bh);
    }

    return 0;
}
EXPORT_SYMBOL_GPL(hw_resume);

static int hw_cdc_reset_resume(struct usb_interface *intf)
{
    return hw_resume (intf);
}

int hw_send_tlp_download_request(struct usb_interface *intf)
{
    struct usb_device *udev = interface_to_usbdev(intf);
    struct usb_host_interface *interface = intf->cur_altsetting;
    struct usbdevfs_ctrltransfer req = {0};
    unsigned char buf[256] = {0};
    int retval = 0;
    req.bRequestType = 0xC0;
    req.bRequest = 0x02;//activating the download tlp feature request
    req.wIndex = interface->desc.bInterfaceNumber;
    req.wValue = 1;
    req.wLength = 1;
    //req.data = buf;
    req.timeout = 1000;
    retval = usb_control_msg(udev, usb_rcvctrlpipe(udev, 0), req.bRequest,  
        req.bRequestType, req.wValue, req.wIndex, 
                buf, req.wLength, req.timeout);
    /*check the TLP feature is activated or not, response value 0x01 indicates success*/
    if (0 < retval && 0x01 == buf[0]){
        return retval;
    }else{
        return 0;
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * probes control interface, claims data interface, collects the bulk
 * endpoints, activates data interface (if needed), maybe sets MTU.
 * all pure cdc
 */
//int hw_generic_cdc_bind(struct hw_cdc_net *dev, struct usb_interface *intf)
#define USB_DEVICE_HUAWEI_DATA 0xFF
static int hw_cdc_bind(struct hw_cdc_net *dev, struct usb_interface *intf)
{
    u8                *buf = intf->cur_altsetting->extra;
    int                len = intf->cur_altsetting->extralen;
    struct usb_interface_descriptor    *d;
    struct hw_dev_state        *info = (void *) &dev->data;
    int                status;
    struct usb_driver        *driver = driver_of(intf);
    int i;
    struct ncm_ctx *ctx = NULL;

    devdbg(dev, "hw_cdc_bind enter\n");
    
    if (sizeof dev->data < sizeof *info){
        return -EDOM;
    }

    dev->ncm_ctx = NULL;
    dev->is_ncm = is_ncm_interface(intf);

    if(dev->is_ncm)
    {
        devdbg(dev, "this is ncm interface\n");
        dev->ncm_ctx = kzalloc(sizeof(struct ncm_ctx), GFP_KERNEL);
        if (dev->ncm_ctx == NULL){
            return -ENOMEM;
        }
        ctx = dev->ncm_ctx;
        ctx->ndev = dev;

        spin_lock_init(&ctx->tx_lock);

        ctx->tx_timer.function = ncm_tx_timer_cb;
        ctx->tx_timer.data = (unsigned long)ctx;
        init_timer(&ctx->tx_timer);


    if(ncm_tx_timeout){
            ctx->tx_timeout_jiffies = msecs_to_jiffies(ncm_tx_timeout);
    }else{
        ctx->tx_timeout_jiffies = 0;
    }

    devdbg(dev,"ctx->tx_timeout_jiffies:%ld",ctx->tx_timeout_jiffies);
    }

    
    memset(info, 0, sizeof *info);
    info->control = intf;
    while (len > 3) {
        if (buf [1] != USB_DT_CS_INTERFACE){
            goto next_desc;
        }

        switch (buf [2]) {
        case USB_CDC_HEADER_TYPE:
            if (info->header) {
                dev_dbg(&intf->dev, "extra CDC header\n");
                goto bad_desc;
            }
            info->header = (void *) buf;
            if (info->header->bLength != sizeof *info->header) {
                dev_dbg(&intf->dev, "CDC header len %u\n",
                    info->header->bLength);
                goto bad_desc;
            }
            break;
        case USB_CDC_UNION_TYPE:
            if (info->u) {
                dev_dbg(&intf->dev, "extra CDC union\n");
                goto bad_desc;
            }
            info->u = (void *) buf;
            if (info->u->bLength != sizeof *info->u) {
                dev_dbg(&intf->dev, "CDC union len %u\n",
                    info->u->bLength);
                goto bad_desc;
            }

            /* we need a master/control interface (what we're
             * probed with) and a slave/data interface; union
             * descriptors sort this all out.
             */
            info->control = usb_ifnum_to_if(dev->udev,
                        info->u->bMasterInterface0);
            info->data = usb_ifnum_to_if(dev->udev,
                        info->u->bSlaveInterface0);
            if (!info->control || !info->data) {
                dev_dbg(&intf->dev,
                    "master #%u/%p slave #%u/%p\n",
                    info->u->bMasterInterface0,
                    info->control,
                    info->u->bSlaveInterface0,
                    info->data);
                goto bad_desc;
            }
            if (info->control != intf) {
                dev_dbg(&intf->dev, "bogus CDC Union\n");
                /* Ambit USB Cable Modem (and maybe others)
                 * interchanges master and slave interface.
                 */
                if (info->data == intf) {
                    info->data = info->control;
                    info->control = intf;
                } else{
                    goto bad_desc;
                }
            }
        
            /*For Jungo solution, the NDIS device has no data interface, so needn't detect data interface*/
            if (HW_JUNGO_BCDDEVICE_VALUE != dev->udev->descriptor.bcdDevice 
             && BINTERFACESUBCLASS != intf->cur_altsetting->desc.bInterfaceSubClass) {
                /* a data interface altsetting does the real i/o */
                d = &info->data->cur_altsetting->desc;
            //if (d->bInterfaceClass != USB_CLASS_CDC_DATA) { /*delete the standard CDC slave class detect*/
            if (d->bInterfaceClass !=  USB_DEVICE_HUAWEI_DATA 
             && d->bInterfaceClass != USB_CLASS_CDC_DATA) {  
               /*Add to detect CDC slave class either Huawei defined or standard*/
                    dev_dbg(&intf->dev, "slave class %u\n",
                        d->bInterfaceClass);
                    goto bad_desc;
                }
            }
            break;
        case USB_CDC_ETHERNET_TYPE:
            if (info->ether) {
                dev_dbg(&intf->dev, "extra CDC ether\n");
                goto bad_desc;
            }
            info->ether = (void *) buf;
            if (info->ether->bLength != sizeof *info->ether) {
                dev_dbg(&intf->dev, "CDC ether len %u\n",
                    info->ether->bLength);
                goto bad_desc;
            }
            dev->hard_mtu = le16_to_cpu(
                        info->ether->wMaxSegmentSize);
            /* because of Zaurus, we may be ignoring the host
             * side link address we were given.
             */
            break;
        case USB_CDC_NCM_TYPE:
            if (dev->ncm_ctx->ncm_desc){
                dev_dbg(&intf->dev, "extra NCM descriptor\n");
            }else{
                dev->ncm_ctx->ncm_desc = (void *)buf;
            }
            break;
        }
next_desc:
        len -= buf [0];    /* bLength */
        buf += buf [0];
    }

    if (!info->header || !info->u || (!dev->is_ncm &&!info->ether) ||
        (dev->is_ncm && !dev->ncm_ctx->ncm_desc)) {
        dev_dbg(&intf->dev, "missing cdc %s%s%s%sdescriptor\n",
            info->header ? "" : "header ",
            info->u ? "" : "union ",
            info->ether ? "" : "ether ",
            dev->ncm_ctx->ncm_desc ? "" : "ncm ");
        goto bad_desc;
    }
    if(dev->is_ncm)
    {
        ctx = dev->ncm_ctx;
        ctx->control = info->control;
        ctx->data = info->data;
        status = cdc_ncm_config(ctx);
        if (status < 0){
            goto error2;
        }

        dev->rx_urb_size = ctx->rx_max_ntb;

        /* We must always have one spare SKB for the current-NTB (of which
         * usbnet has no account)
         */
        ctx->skb_pool_size = TX_QLEN_NCM;

        ctx->skb_pool = kzalloc(sizeof(struct sk_buff *) * ctx->skb_pool_size,
            GFP_KERNEL);
        if (ctx->skb_pool == NULL) {
            dev_dbg(&intf->dev, "failed allocating the SKB pool\n");
            goto error2;
        }

        for (i = 0; i < ctx->skb_pool_size; i++) {
            ctx->skb_pool[i] = alloc_skb(ctx->tx_max_ntb, GFP_KERNEL);
            if (ctx->skb_pool[i] == NULL) {
                dev_dbg(&intf->dev, "failed allocating an SKB for the "
                    "SKB pool\n");
                goto error3;
            }
        }
        
        ntb_clear(&ctx->curr_ntb);
    
    }


    /*if the NDIS device is not Jungo solution, then assume that it has the data interface, and claim for it*/
    if (HW_JUNGO_BCDDEVICE_VALUE != dev->udev->descriptor.bcdDevice 
     && BINTERFACESUBCLASS != intf->cur_altsetting->desc.bInterfaceSubClass)
    {
        /* claim data interface and set it up ... with side effects.
          * network traffic can't flow until an altsetting is enabled.
          */
          
         /*Begin:add by h00122846 for ndis bind error at 20101106*/
        if(info->data->dev.driver != NULL)
        {
            usb_driver_release_interface(driver, info->data);
        }
                /*End:add by h00122846 for ndis bind error at 20101106*/

        status = usb_driver_claim_interface(driver, info->data, dev);
        if (status < 0){
            return status;
        }
    }

    status = hw_get_endpoints(dev, info->data);
    if (status < 0) {
        /* ensure immediate exit from hw_disconnect */
        goto error3;
    }

    /* status endpoint: optional for CDC Ethernet, */
    dev->status = NULL;
    if (HW_JUNGO_BCDDEVICE_VALUE == dev->udev->descriptor.bcdDevice 
     || BINTERFACESUBCLASS == intf->cur_altsetting->desc.bInterfaceSubClass 
     || info->control->cur_altsetting->desc.bNumEndpoints == 1)
    {
        struct usb_endpoint_descriptor    *desc;
        dev->status = &info->control->cur_altsetting->endpoint [0];
        desc = &dev->status->desc;
        if (((desc->bmAttributes  & USB_ENDPOINT_XFERTYPE_MASK) != USB_ENDPOINT_XFER_INT)
                || ((desc->bEndpointAddress & USB_ENDPOINT_DIR_MASK) != USB_DIR_IN)
                || (le16_to_cpu(desc->wMaxPacketSize)
                    < sizeof(struct usb_cdc_notification))
                || !desc->bInterval) {
            printk(KERN_ERR"fxz-%s:bad notification endpoint\n", __func__);
            dev->status = NULL;
        }
    }
    
    return hw_get_ethernet_addr(dev);
    
error3:
    if(dev->is_ncm){
        for ( i = 0; i < ctx->skb_pool_size && ctx->skb_pool[i]; i++){
            dev_kfree_skb_any(ctx->skb_pool[i]);
        }
        kfree(ctx->skb_pool);
    }
error2:
    /* ensure immediate exit from cdc_disconnect */
    usb_set_intfdata(info->data, NULL);
    usb_driver_release_interface(driver_of(intf), info->data);

    if(dev->ncm_ctx){
        kfree(dev->ncm_ctx);    
    }
    return status;  

bad_desc:
    devinfo(dev, "bad CDC descriptors\n");
    return -ENODEV;
}

void hw_cdc_unbind(struct hw_cdc_net *dev, struct usb_interface *intf)
{
    struct hw_dev_state        *info = (void *) &dev->data;
    struct usb_driver        *driver = driver_of(intf);
    int i;

    /* disconnect master --> disconnect slave */
    if (intf == info->control && info->data) {
        /* ensure immediate exit from usbnet_disconnect */
        usb_set_intfdata(info->data, NULL);
        usb_driver_release_interface(driver, info->data);
        info->data = NULL;
    }

    /* and vice versa (just in case) */
    else if (intf == info->data && info->control) {
        /* ensure immediate exit from usbnet_disconnect */
        usb_set_intfdata(info->control, NULL);
        usb_driver_release_interface(driver, info->control);
        info->control = NULL;
    }
    if(dev->is_ncm && dev->ncm_ctx){
        del_timer_sync(&dev->ncm_ctx->tx_timer);

        ntb_free_dgram_list(&dev->ncm_ctx->curr_ntb);
        for (i = 0; i < dev->ncm_ctx->skb_pool_size; i++){
            dev_kfree_skb_any(dev->ncm_ctx->skb_pool[i]);
        }
        kfree(dev->ncm_ctx->skb_pool);
        kfree(dev->ncm_ctx);
        dev->ncm_ctx = NULL;
    }
    
    
}
EXPORT_SYMBOL_GPL(hw_cdc_unbind);


/*-------------------------------------------------------------------------
 *
 * Communications Device Class, Ethernet Control model
 *
 * Takes two interfaces.  The DATA interface is inactive till an altsetting
 * is selected.  Configuration data includes class descriptors.  There's
 * an optional status endpoint on the control interface.
 *
 * This should interop with whatever the 2.4 "CDCEther.c" driver
 * (by Brad Hards) talked with, with more functionality.
 *
 *-------------------------------------------------------------------------*/

static void dumpspeed(struct hw_cdc_net *dev, __le32 *speeds)
{
    if (netif_msg_timer(dev)){
        devinfo(dev, "link speeds: %u kbps up, %u kbps down",
            __le32_to_cpu(speeds[0]) / 1000,
        __le32_to_cpu(speeds[1]) / 1000);
    }
}

static inline int hw_get_ethernet_addr(struct hw_cdc_net *dev)
{

    dev->net->dev_addr[0] = 0x00;
    dev->net->dev_addr[1] = 0x1e;

    dev->net->dev_addr[2] = 0x10;
    dev->net->dev_addr[3] = 0x1f;
    dev->net->dev_addr[4] = 0x00;
    dev->net->dev_addr[5] = 0x01;/*change 0x04 into 0x01 20100129*/

    return 0;
}


enum {WRITE_REQUEST = 0x21, READ_RESPONSE = 0xa1};
#define HW_CDC_OK 0
#define HW_CDC_FAIL -1
/*-------------------------------------------------------------------------*/
/*The ioctl is called to send the qmi request to the device 
  * or get the qmi response from the device*/
static int hw_cdc_ioctl (struct usb_interface *intf, unsigned int code,
            void *buf)
{
    struct usb_device *udev = interface_to_usbdev(intf);
    struct hw_cdc_net *hwnet = (struct hw_cdc_net *)dev_get_drvdata(&intf->dev);
    struct usb_host_interface *interface = intf->cur_altsetting;
    struct usbdevfs_ctrltransfer *req = (struct usbdevfs_ctrltransfer *)buf;
    char *pbuf = NULL;
    int ret = -1;
    if (HW_JUNGO_BCDDEVICE_VALUE != hwnet->udev->descriptor.bcdDevice 
     && BINTERFACESUBCLASS != intf->cur_altsetting->desc.bInterfaceSubClass){
        if (1 == hwnet->qmi_sync) {
            deverr(hwnet, "%s: The ndis port is busy.", __FUNCTION__);
            return HW_CDC_FAIL;
        }
    }

    if (USBDEVFS_CONTROL != code || NULL == req){
        deverr(hwnet, "%s: The request is not supported.", __FUNCTION__);
        return HW_CDC_FAIL;
    }

    if (0 < req->wLength){
        pbuf = (char *)kmalloc(req->wLength + 1, GFP_KERNEL);
        if (NULL == pbuf){
            deverr(hwnet, "%s: Kmalloc the buffer failed.", __FUNCTION__);
            return HW_CDC_FAIL;
        }
        memset(pbuf, 0, req->wLength);
    }

    switch (req->bRequestType)
    {
        case WRITE_REQUEST:
        {
            if (NULL != req->data && 0 < req->wLength){
                if (copy_from_user(pbuf, req->data, req->wLength)){
                    deverr(hwnet, "usbnet_cdc_ioctl: copy_from_user failed");
                    goto op_error;
                }
                
            }else{
                pbuf = NULL;
                req->wLength = 0;
            }
            pbuf[req->wLength] = 0;
            ret = usb_control_msg(udev, usb_sndctrlpipe(udev, 0), req->bRequest, 
                    req->bRequestType, req->wValue, interface->desc.bInterfaceNumber, 
                    pbuf, req->wLength, req->timeout);
            break;
        }
        case READ_RESPONSE:
        {
            if (NULL == req->data || 0 >= req->wLength || NULL == pbuf){
                deverr(hwnet, "%s: The buffer is null, can not read the response.",
                       __FUNCTION__);
                goto op_error;
            }
            ret = usb_control_msg(udev, 
                                  usb_rcvctrlpipe(udev, 0), 
                                  req->bRequest, 
                                  req->bRequestType, 
                                  req->wValue, 
                                  interface->desc.bInterfaceNumber, 
                                  pbuf, 
                                  req->wLength, 
                                  req->timeout);

            if (0 < ret){
                if (HW_JUNGO_BCDDEVICE_VALUE != hwnet->udev->descriptor.bcdDevice 
                 && BINTERFACESUBCLASS != intf->cur_altsetting->desc.bInterfaceSubClass)
            {
                    /*check the connection indication*/
                    if (0x04 == pbuf[6] && 0x22 == pbuf[9] && 0x00 == pbuf[10]){
                        if (0x02 == pbuf[16]){
                            if (hwnet){
                                netif_carrier_on(hwnet->net);
                            }
                            }else{
                                if (hwnet){
                                    netif_carrier_off(hwnet->net);
                            }
                        }
                    }
                }
                if (copy_to_user(req->data, pbuf, req->wLength)){
                    deverr(hwnet, "%s: copy_from_user failed", __FUNCTION__);
                    goto op_error;
                }    
            }
            break;
        }
        default:
            break;
    }

    if (NULL != pbuf){
        kfree(pbuf);
        pbuf = NULL;
    }

    return HW_CDC_OK;

op_error:
    if (NULL != pbuf){
        kfree(pbuf);
        pbuf = NULL;
    }
    return HW_CDC_FAIL;

}
/* delete by lKF36757 2011/12/26,prevent hilink load hw_cdc_driver.ko*/
/*
 *#define    HUAWEI_ETHER_INTERFACE \
 *    .bInterfaceClass    = USB_CLASS_COMM, \
 *    .bInterfaceSubClass    = USB_CDC_SUBCLASS_ETHERNET, \
 *    .bInterfaceProtocol    = USB_CDC_PROTO_NONE
 */


#define    HUAWEI_NDIS_INTERFACE \
    .bInterfaceClass    = USB_CLASS_COMM, \
    .bInterfaceSubClass    = USB_CDC_SUBCLASS_ETHERNET, \
    .bInterfaceProtocol    = 0xff

#define    HUAWEI_NCM_INTERFACE \
    .bInterfaceClass    = USB_CLASS_COMM, \
    .bInterfaceSubClass    = 0x0d, \
    .bInterfaceProtocol    = 0xff

#define    HUAWEI_NCM_INTERFACE2 \
    .bInterfaceClass    = USB_CLASS_COMM, \
    .bInterfaceSubClass    = 0x0d, \
    .bInterfaceProtocol    = 0x00


/*Add for PID optimized fangxz 20091105*/
#define    HUAWEI_NDIS_OPTIMIZED_INTERFACE \
    .bInterfaceClass    = 0xFF, \
    .bInterfaceSubClass    = 0x01, \
    .bInterfaceProtocol    = 0x09
    
/*Add for PID optimized lxz 20120508*/
#define    HUAWEI_NDIS_OPTIMIZED_INTERFACE_JUNGO \
    .bInterfaceClass    = 0xFF, \
    .bInterfaceSubClass    = 0x02, \
    .bInterfaceProtocol    = 0x09

/*Add for PID optimized marui 20100628*/
#define    HUAWEI_NDIS_OPTIMIZED_INTERFACE_VDF \
    .bInterfaceClass    = 0xFF, \
    .bInterfaceSubClass    = 0x01, \
    .bInterfaceProtocol    = 0x39
    
/*Add for PID optimized lxz 20120508*/
#define    HUAWEI_NDIS_OPTIMIZED_INTERFACE_VDF_JUNGO \
    .bInterfaceClass    = 0xFF, \
    .bInterfaceSubClass    = 0x02, \
    .bInterfaceProtocol    = 0x39

/*Add for PID optimized lxz 20120508*/
#define    HUAWEI_NDIS_SINGLE_INTERFACE \
    .bInterfaceClass    = 0xFF, \
    .bInterfaceSubClass    = 0x01, \
    .bInterfaceProtocol    = 0x07
    
/*Add for PID optimized marui 20100811*/
#define    HUAWEI_NDIS_SINGLE_INTERFACE_JUNGO \
    .bInterfaceClass    = 0xFF, \
    .bInterfaceSubClass    = 0x02, \
    .bInterfaceProtocol    = 0x07

/*Add for PID optimized lxz 20120508*/    
#define    HUAWEI_NDIS_SINGLE_INTERFACE_VDF \
    .bInterfaceClass    = 0xFF, \
    .bInterfaceSubClass    = 0x01, \
    .bInterfaceProtocol    = 0x37    
    
/*Add for PID optimized marui 20100811*/
#define    HUAWEI_NDIS_SINGLE_INTERFACE_VDF_JUNGO \
    .bInterfaceClass    = 0xFF, \
    .bInterfaceSubClass    = 0x02, \
    .bInterfaceProtocol    = 0x37

/*Add for PID optimized lxz 20120508*/
#define    HUAWEI_NCM_OPTIMIZED_INTERFACE \
    .bInterfaceClass    = 0xFF, \
    .bInterfaceSubClass    = 0x01, \
    .bInterfaceProtocol    = 0x16
    
/*Add for PID optimized liaojianping 20100811*/
#define    HUAWEI_NCM_OPTIMIZED_INTERFACE_JUNGO \
    .bInterfaceClass    = 0xFF, \
    .bInterfaceSubClass    = 0x02, \
    .bInterfaceProtocol    = 0x16
    
/*Add for PID optimized lxz 20120508*/
#define    HUAWEI_NCM_OPTIMIZED_INTERFACE_VDF \
    .bInterfaceClass    = 0xFF, \
    .bInterfaceSubClass    = 0x01, \
    .bInterfaceProtocol    = 0x46
    
#define    HUAWEI_NCM_OPTIMIZED_INTERFACE_VDF_JUNGO \
    .bInterfaceClass    = 0xFF, \
    .bInterfaceSubClass    = 0x02, \
    .bInterfaceProtocol    = 0x46
    
/*Add for PID optimized xiaruihu 20110825*/
#define    HUAWEI_INTERFACE_NDIS_NO_3G_JUNGO \
    .bInterfaceClass    = 0xFF, \
    .bInterfaceSubClass    = 0x02, \
    .bInterfaceProtocol    = 0x11
    
#define    HUAWEI_INTERFACE_NDIS_NO_3G_QUALCOMM \
    .bInterfaceClass    = 0xFF, \
    .bInterfaceSubClass    = 0x01, \
    .bInterfaceProtocol    = 0x11
    
/*Add for PID optimized xiaruihu 20111008*/
#define    HUAWEI_INTERFACE_NDIS_HW_QUALCOMM \
    .bInterfaceClass    = 0xFF, \
    .bInterfaceSubClass    = 0x01, \
    .bInterfaceProtocol    = 0x67
    
#define    HUAWEI_INTERFACE_NDIS_CONTROL_QUALCOMM \
    .bInterfaceClass    = 0xFF, \
    .bInterfaceSubClass    = 0x01, \
    .bInterfaceProtocol    = 0x69

#define    HUAWEI_INTERFACE_NDIS_NCM_QUALCOMM \
    .bInterfaceClass    = 0xFF, \
    .bInterfaceSubClass    = 0x01, \
    .bInterfaceProtocol    = 0x76
    
#define    HUAWEI_INTERFACE_NDIS_HW_JUNGO \
    .bInterfaceClass    = 0xFF, \
    .bInterfaceSubClass    = 0x02, \
    .bInterfaceProtocol    = 0x67
    
#define    HUAWEI_INTERFACE_NDIS_CONTROL_JUNGO \
    .bInterfaceClass    = 0xFF, \
    .bInterfaceSubClass    = 0x02, \
    .bInterfaceProtocol    = 0x69

#define    HUAWEI_INTERFACE_NDIS_NCM_JUNGO \
    .bInterfaceClass    = 0xFF, \
    .bInterfaceSubClass    = 0x02, \
    .bInterfaceProtocol    = 0x76
    

static const struct usb_device_id    hw_products [] = {
    /*删除对PRODUCT ID的比较，默认能够支持所有HUAWEI_ETHER_INTERFACE 接口类型的NDIS设备*/
    /* delete by lKF36757 2011/12/26,prevent hilink load hw_cdc_driver.ko*/
    /*
    {
        .match_flags    =   USB_DEVICE_ID_MATCH_INT_INFO
              | USB_DEVICE_ID_MATCH_VENDOR,
        .idVendor        = 0x12d1,
        HUAWEI_ETHER_INTERFACE,
    },*/
    {
        .match_flags    =   USB_DEVICE_ID_MATCH_INT_INFO
              | USB_DEVICE_ID_MATCH_VENDOR,
        .idVendor        = 0x12d1,
        HUAWEI_NDIS_INTERFACE,
    },
    /*Add for PID optimized fangxz 20091105*/
    {
        .match_flags    =   USB_DEVICE_ID_MATCH_INT_INFO
              | USB_DEVICE_ID_MATCH_VENDOR,
        .idVendor        = 0x12d1,
        HUAWEI_NDIS_OPTIMIZED_INTERFACE,
    },
    /*Add for VDF PID optimized marui 20100628*/
    {
        .match_flags    =   USB_DEVICE_ID_MATCH_INT_INFO
              | USB_DEVICE_ID_MATCH_VENDOR,
        .idVendor        = 0x12d1,
        HUAWEI_NDIS_OPTIMIZED_INTERFACE_VDF,
    },
    /*Add for PID optimized marui 20100811*/
    {
        .match_flags    =   USB_DEVICE_ID_MATCH_INT_INFO
              | USB_DEVICE_ID_MATCH_VENDOR,
        .idVendor        = 0x12d1,
        HUAWEI_NDIS_OPTIMIZED_INTERFACE_JUNGO,
    },
    /*Add for VDF PID optimized marui 20100811*/
    {
        .match_flags    =   USB_DEVICE_ID_MATCH_INT_INFO
              | USB_DEVICE_ID_MATCH_VENDOR,
        .idVendor        = 0x12d1,
        HUAWEI_NDIS_OPTIMIZED_INTERFACE_VDF_JUNGO,
    },
    /*Add for NCM PID optimized lxz 20120508*/
    {
        .match_flags    =   USB_DEVICE_ID_MATCH_INT_INFO
              | USB_DEVICE_ID_MATCH_VENDOR,
        .idVendor        = 0x12d1,
        HUAWEI_NCM_OPTIMIZED_INTERFACE,
    },
    /*Add for NCM PID optimized liaojianping 20100911*/
    {
        .match_flags    =   USB_DEVICE_ID_MATCH_INT_INFO
              | USB_DEVICE_ID_MATCH_VENDOR,
        .idVendor        = 0x12d1,
        HUAWEI_NCM_OPTIMIZED_INTERFACE_JUNGO,
    },
    /*Add for VDF NCM PID optimized lxz 20120508*/
    {
        .match_flags    =   USB_DEVICE_ID_MATCH_INT_INFO
              | USB_DEVICE_ID_MATCH_VENDOR,
        .idVendor        = 0x12d1,
        HUAWEI_NCM_OPTIMIZED_INTERFACE_VDF,
    },
    /*Add for VDF NCM PID optimized liaojianping 20100911*/
    {
        .match_flags    =   USB_DEVICE_ID_MATCH_INT_INFO
              | USB_DEVICE_ID_MATCH_VENDOR,
        .idVendor        = 0x12d1,
        HUAWEI_NCM_OPTIMIZED_INTERFACE_VDF_JUNGO,
    },
    /*Add for ncm liaojianping 20100911*/
    {
        .match_flags    =   USB_DEVICE_ID_MATCH_INT_INFO
              | USB_DEVICE_ID_MATCH_VENDOR,
        .idVendor        = 0x12d1,
        HUAWEI_NCM_INTERFACE,
    },
            /*Add for VDF NCM PID optimized liaojianping 20100911*/
    {
        .match_flags    =   USB_DEVICE_ID_MATCH_INT_INFO
              | USB_DEVICE_ID_MATCH_VENDOR,
        .idVendor        = 0x12d1,
        HUAWEI_NCM_INTERFACE2,
    },
        /*Add for PID optimized xiaruihu 20110825*/
    {
        .match_flags    =   USB_DEVICE_ID_MATCH_INT_INFO
              | USB_DEVICE_ID_MATCH_VENDOR,
        .idVendor        = 0x12d1,
        HUAWEI_INTERFACE_NDIS_NO_3G_JUNGO
    },
    {
        .match_flags    =   USB_DEVICE_ID_MATCH_INT_INFO
              | USB_DEVICE_ID_MATCH_VENDOR,
        .idVendor        = 0x12d1,
        HUAWEI_INTERFACE_NDIS_NO_3G_QUALCOMM
    },
    /*Add for PID optimized xiaruihu 20111008*/
    {
        .match_flags    =   USB_DEVICE_ID_MATCH_INT_INFO
              | USB_DEVICE_ID_MATCH_VENDOR,
        .idVendor        = 0x12d1,
        HUAWEI_INTERFACE_NDIS_HW_QUALCOMM
    },
    {
        .match_flags    =   USB_DEVICE_ID_MATCH_INT_INFO
              | USB_DEVICE_ID_MATCH_VENDOR,
        .idVendor        = 0x12d1,
        HUAWEI_INTERFACE_NDIS_HW_JUNGO
    },    
    {
        .match_flags    =   USB_DEVICE_ID_MATCH_INT_INFO
              | USB_DEVICE_ID_MATCH_VENDOR,
        .idVendor        = 0x12d1,
        HUAWEI_INTERFACE_NDIS_CONTROL_QUALCOMM 
    },    
    {
        .match_flags    =   USB_DEVICE_ID_MATCH_INT_INFO
              | USB_DEVICE_ID_MATCH_VENDOR,
        .idVendor        = 0x12d1,
        HUAWEI_INTERFACE_NDIS_CONTROL_JUNGO
    },    
    {
        .match_flags    =   USB_DEVICE_ID_MATCH_INT_INFO
              | USB_DEVICE_ID_MATCH_VENDOR,
        .idVendor        = 0x12d1,
        HUAWEI_INTERFACE_NDIS_NCM_QUALCOMM 
    },    
    {
        .match_flags    =   USB_DEVICE_ID_MATCH_INT_INFO
              | USB_DEVICE_ID_MATCH_VENDOR,
        .idVendor        = 0x12d1,
        HUAWEI_INTERFACE_NDIS_NCM_JUNGO
    },
    {
        .match_flags    =   USB_DEVICE_ID_MATCH_INT_INFO
              | USB_DEVICE_ID_MATCH_VENDOR,
        .idVendor        = 0x12d1,
        HUAWEI_NDIS_SINGLE_INTERFACE
    },
    {
        .match_flags    =   USB_DEVICE_ID_MATCH_INT_INFO
              | USB_DEVICE_ID_MATCH_VENDOR,
        .idVendor        = 0x12d1,
        HUAWEI_NDIS_SINGLE_INTERFACE_JUNGO
    },
    {
        .match_flags    =   USB_DEVICE_ID_MATCH_INT_INFO
              | USB_DEVICE_ID_MATCH_VENDOR,
        .idVendor        = 0x12d1,
        HUAWEI_NDIS_SINGLE_INTERFACE_VDF
    },
    {
        .match_flags    =   USB_DEVICE_ID_MATCH_INT_INFO
              | USB_DEVICE_ID_MATCH_VENDOR,
        .idVendor        = 0x12d1,
        HUAWEI_NDIS_SINGLE_INTERFACE_VDF_JUNGO
    },            
    { },        // END
};
MODULE_DEVICE_TABLE(usb, hw_products);

static int hw_cdc_reset_resume(struct usb_interface *intf);
static struct usb_driver hw_ether_driver = {
    .name =        "huawei_ether",
    .id_table =    hw_products,
    .probe =    hw_cdc_probe,
    .disconnect =    hw_disconnect,
#if !(LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 36))
    .unlocked_ioctl        =    hw_cdc_ioctl,
#else
        .ioctl = hw_cdc_ioctl,
#endif
    .suspend =    hw_suspend,
    .resume =    hw_resume,
    .reset_resume = hw_cdc_reset_resume,
};


static void hw_cdc_status(struct hw_cdc_net *dev, struct urb *urb)
{
    struct usb_cdc_notification    *event;

    if (urb->actual_length < sizeof *event){
        return;
    }

    /* SPEED_CHANGE can get split into two 8-byte packets */
    if (test_and_clear_bit(EVENT_STS_SPLIT, &dev->flags)) {
        devdbg(dev, "The speed is changed by status event");
        dumpspeed(dev, (__le32 *) urb->transfer_buffer);
        return;
    }

    event = urb->transfer_buffer;
    switch (event->bNotificationType) {
    case USB_CDC_NOTIFY_NETWORK_CONNECTION:
        if (netif_msg_timer(dev)){
            devdbg(dev, "CDC: carrier %s",
                    event->wValue ? "on" : "off");
        }
        if (event->wValue){
            netif_carrier_on(dev->net);
            devinfo(dev, "CDC: network connection: connected\n");
        }else{
            netif_carrier_off(dev->net);
            devinfo(dev, "CDC: network connection: disconnected\n");
        }

        break;
    case USB_CDC_NOTIFY_SPEED_CHANGE:    /* tx/rx rates */
        if (netif_msg_timer(dev)){
            devdbg(dev, "CDC: speed change (len %d)",
                    urb->actual_length);
        }
        if (urb->actual_length != (sizeof *event + 8)){
            set_bit(EVENT_STS_SPLIT, &dev->flags);
        }else{
            dumpspeed(dev, (__le32 *) &event[1]);
        }
        break;

    case USB_CDC_NOTIFY_RESPONSE_AVAILABLE:
    {
        break;
    }
        
    default:
        devdbg(dev, "%s: CDC: unexpected notification %02x!", __FUNCTION__,
                 event->bNotificationType);
        break;
    }
}


static int __init hw_cdc_init(void)
{
    BUG_ON((sizeof(((struct hw_cdc_net *)0)->data)
            < sizeof(struct hw_dev_state)));

    return usb_register(&hw_ether_driver);
}
fs_initcall(hw_cdc_init);

static int hw_send_qmi_request(struct usb_interface *intf, 
                unsigned char *snd_req, int snd_len, 
                unsigned char *read_resp, int resp_len);
static int hw_send_qmi_request_no_resp(struct usb_interface *intf, 
                unsigned char *snd_req, int snd_len, 
                unsigned char *read_resp, int resp_len);


//int hw_check_conn_status(struct usb_interface *intf)
static void hw_cdc_check_status_work(struct work_struct *work)

{
    //struct hw_cdc_net *net = usb_get_intfdata(intf);
    //usb_device *udev = interface_to_usbdev(intf);
    struct hw_cdc_net *dev = container_of(work, struct hw_cdc_net, status_work.work);

    int ret;
    int repeat = 0;
    unsigned char resp_buf[56] = {0};
    unsigned char client_id_req[0x10] = {0x01, 0x0f, 0x00, 0x00, 0x00, 
                                        0x00, 0x00, 0x06, 0x22, 0x00, 
                                        0x04, 0x00, 0x01, 0x01, 0x00, 0x01};
    unsigned char rel_client_id_req[0x11] = {0x01, 0x10, 0x00, 0x00, 0x00, 
                                        0x00,  0x00, 0x00, 0x23,0x00, 
                                        0x05, 0x00, 0x01, 0x02, 0x00, 
                                        0x01, 0x00};
    unsigned char status_req[13] = {0x01, 0x0c, 0x00, 0x00, 0x01, 
                    0x00,  0x00, 0x02, 0x00, 
                    0x22, 0x00, 0x00, 0x00};
    unsigned char set_instance_req[0x10] = {0x01, 0x0f, 0x00, 0x00, 0x00, 
                                        0x00, 0x00, 0x06, 0x20, 0x00, 
                                        0x04, 0x00, 0x01, 0x01, 0x00, 0x00};
    dev->qmi_sync = 1;    
    
     hw_send_qmi_request_no_resp(dev->intf, set_instance_req, 0x10, resp_buf, 56);

    ret = hw_send_qmi_request(dev->intf, client_id_req, 0x10, resp_buf, 56);
    if (0 == ret){
        printk(KERN_ERR"%s: Get client ID failed\n", __FUNCTION__);
        goto failed;
    }
    status_req[5] = resp_buf[23];
    memset(resp_buf, 0, 56 * sizeof (unsigned char));

    //for (repeat = 0; repeat < 3; repeat ++)
    for (repeat = 0; repeat < 3; repeat++)
    {
        ret = hw_send_qmi_request(dev->intf, status_req, 13, resp_buf, 56);
        if (0 == ret){
            printk(KERN_ERR"%s: Get connection status failed\n", __FUNCTION__);
            continue;
        }

        if (0x02 == resp_buf[23]){
            printk(KERN_ERR"%s: carrier on\n", __FUNCTION__);
            netif_carrier_on(dev->net);
            break;    
        } else {

            printk(KERN_ERR"%s: carrier off\n", __FUNCTION__);
            //netif_carrier_off(dev->net);
        }
    }
failed:
    rel_client_id_req[0x0f] = 0x02;
    rel_client_id_req[0x10] = status_req[5];
    memset(resp_buf, 0, 56 * sizeof (unsigned char));

    ret = hw_send_qmi_request_no_resp(dev->intf, rel_client_id_req, 0x11, resp_buf, 56);
    
    dev->qmi_sync = 0;
    cancel_delayed_work(&dev->status_work);
    //memset(resp_buf, 0, 56 * sizeof (unsigned char));
    return;
    
}
static int hw_send_qmi_request_no_resp(struct usb_interface *intf, 
                unsigned char *snd_req, int snd_len, 
                unsigned char *read_resp, int resp_len)
{
    int ret;
    int index = 0;
    struct usb_device *udev = interface_to_usbdev(intf);
    for (index = 0; index < 3; index++)
{
        ret = usb_control_msg(udev, usb_sndctrlpipe(udev, 0), 0x00, 
                    0x21, 0x00, intf->cur_altsetting->desc.bInterfaceNumber, 
                    snd_req, snd_len, 5000);
        if (ret < 0){
            printk(KERN_ERR"%s: send the qmi request failed\n", __FUNCTION__);
            continue;
        }
        else {
            break;
        }
    }
    return ret;
}

static int hw_send_qmi_request(struct usb_interface *intf, 
                unsigned char *snd_req, int snd_len, 
                unsigned char *read_resp, int resp_len)
{
    int ret;
    int index = 0;
    struct usb_device *udev = interface_to_usbdev(intf);
    struct hw_cdc_net *net = usb_get_intfdata(intf);
    
    ret = usb_control_msg(udev, usb_sndctrlpipe(udev, 0), 0x00, 
                    0x21, 0x00, intf->cur_altsetting->desc.bInterfaceNumber, 
                    snd_req, snd_len, 5000);

    if (ret < 0){
        printk(KERN_ERR"%s: send the qmi request failed\n", __FUNCTION__);
        return ret;
    }

    while(index < 10){
        ret = usb_control_msg(udev, usb_rcvctrlpipe(udev, 0), 0x01, 
                    0xA1, 0x00, intf->cur_altsetting->desc.bInterfaceNumber, 
                    read_resp, resp_len, 1000);
        if (ret <= 0){
            printk(KERN_ERR"%s: %d Get response failed\n", __FUNCTION__, index);
            msleep(10);    
        } else {
            if (0x00 == read_resp[4]){
                if (0x01 == read_resp[6] && snd_req[5] == read_resp[5]
                    && snd_req[8] == read_resp[8] && snd_req[9] == read_resp[9]) {
                    ret = 1;
                    break;
                }
            } else if (0x01 == read_resp[4]) {
                if (0x02 == read_resp[6] && snd_req[5] == read_resp[5]
                    && snd_req[9] == read_resp[9] && snd_req[10] == read_resp[10]) {
                    printk(KERN_ERR"%s: get the conn status req=%02x resp\n",
                            __FUNCTION__, snd_req[9]);
                    ret = 1;
                    break;
                }
            } else if (0x04 == read_resp[4]){
                if (snd_req[9] == read_resp[9] && snd_req[10] == read_resp[10] 
                 && 0x02 == read_resp[16]) {
                    printk(KERN_ERR"%s: get the conn status ind= carrier on\n",
                           __FUNCTION__);
                    netif_carrier_on(net->net);
                }
            }
        }
        //index ++;
        index++;
        continue;
    }

    if (index >= 10){
        ret = 0;
    }
    return ret;
}
static void __exit hw_cdc_exit(void)
{
     usb_deregister(&hw_ether_driver);
}
module_exit(hw_cdc_exit);


MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPL");
