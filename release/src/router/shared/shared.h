#ifndef __SHARED_H__
#define __SHARED_H__

#include <tomato_profile.h>

#include <netinet/in.h>
#include <stdint.h>
#include <errno.h>

#include <mntent.h>	// !!TB

#define Y2K			946684800UL		// seconds since 1970

#define ASIZE(array)	(sizeof(array) / sizeof(array[0]))

//version.c
extern const char *tomato_version;
extern const char *tomato_buildtime;


// misc.c
#define	WP_DISABLED		0		// order must be synced with def in misc.c
#define	WP_STATIC		1
#define WP_DHCP			2
#define	WP_L2TP			3
#define	WP_PPPOE		4
#define	WP_PPTP			5

enum {
	ACT_IDLE,
	ACT_TFTP_UPGRADE_UNUSED,
	ACT_WEB_UPGRADE,
	ACT_WEBS_UPGRADE_UNUSED,
	ACT_SW_RESTORE,
	ACT_HW_RESTORE,
	ACT_ERASE_NVRAM,
	ACT_NVRAM_COMMIT,
	ACT_REBOOT,
	ACT_UNKNOWN
};

typedef struct {
	int count;
	struct in_addr dns[3];
} dns_list_t;

extern int get_wan_proto(void);
extern int using_dhcpc(void);
extern void notice_set(const char *path, const char *format, ...);
extern int check_wanup(void);
extern const dns_list_t *get_dns(void);
extern void set_action(int a);
extern int check_action(void);
extern int wait_action_idle(int n);
extern int wl_client(void);
extern const char *get_wanip(void);
extern long get_uptime(void);
extern int get_radio(void);
extern void set_radio(int on);
extern int nvram_get_int(const char *key);
//	extern long nvram_xget_long(const char *name, long min, long max, long def);
extern int nvram_get_file(const char *key, const char *fname, int max);
extern int nvram_set_file(const char *key, const char *fname, int max);
extern int nvram_contains_word(const char *key, const char *word);
extern int nvram_is_empty(const char *key);
extern void nvram_commit_x(void);
extern int connect_timeout(int fd, const struct sockaddr *addr, socklen_t len, int timeout);
//!!TB
extern char *detect_fs_type(char *device);
extern struct mntent *findmntents(char *file, int swp,
	int (*func)(struct mntent *mnt, uint flags), uint flags);
extern int find_label(char *mnt_dev, char *the_label);
extern int file_lock(char *tag);
extern void file_unlock(int lockfd);
extern void add_remove_usbhost(char *host, int add);

#define DEV_DISCS_ROOT	"/dev/discs"

/* Flags used in exec_for_host calls
 */
#define EFH_1ST_HOST	0x00000001	/* func is called for the 1st time for this host */
#define EFH_1ST_DISC	0x00000002	/* func is called for the 1st time for this disc */
#define EFH_HUNKNOWN	0x00000004	/* host is unknown */
#define EFH_USER	0x00000008	/* process is user-initiated - either via Web GUI or a script */

typedef int (*host_exec)(char *dev_name, int host_num, int disc_num, int part_num, uint flags);
extern int exec_for_host(int host, int when_to_update, uint flags, host_exec func);

// id.c
enum {
	MODEL_UNKNOWN,
	MODEL_WRT54G,
	MODEL_WRTSL54GS,
	MODEL_WHRG54S,
	MODEL_WHRHPG54,
	MODEL_WR850GV1,
	MODEL_WR850GV2,
	MODEL_WZRG54,
	MODEL_WL500GP,
	MODEL_WL500GPv2,
	MODEL_WL500GE,
	MODEL_WL520GU,
	MODEL_WBRG54,
	MODEL_WBR2G54,
	MODEL_WX6615GT,
	MODEL_WZRHPG54,
	MODEL_WZRRSG54,
	MODEL_WZRRSG54HP,
	MODEL_WVRG54NF,
	MODEL_WHR2A54G54,
	MODEL_WHR3AG54,
	MODEL_RT390W,
	MODEL_MN700,
	MODEL_WRH54G,
	MODEL_WHRG125,
	MODEL_WZRG108,
	MODEL_WTR54GS,
	MODEL_WR100
	
#if TOMATO_N
	,
	MODEL_WZRG300N,
	MODEL_WRT300N
#endif
};

enum {
	HW_BCM4702,
	HW_BCM4712,
	HW_BCM5325E,
	HW_BCM4704_BCM5325F,
	HW_BCM5352E,
	HW_BCM5354G,
	HW_BCM4712_BCM5325E,
	HW_BCM4704_BCM5325F_EWC,
	HW_BCM4705L_BCM5325E_EWC,
	HW_BCM5350,
	HW_UNKNOWN
};

#define SUP_SES			(1 << 0)
#define SUP_BRAU		(1 << 1)
#define SUP_AOSS_LED	(1 << 2)
#define SUP_WHAM_LED	(1 << 3)
#define SUP_HPAMP		(1 << 4)
#define SUP_NONVE		(1 << 5)
#define SUP_80211N		(1 << 6)

extern int check_hw_type(void);
//	extern int get_hardware(void) __attribute__ ((weak, alias ("check_hw_type")));
extern int get_model(void);
extern int supports(unsigned long attr);



// process.c
extern char *psname(int pid, char *buffer, int maxlen);
extern int pidof(const char *name);
extern int killall(const char *name, int sig);


// files.c
#define FW_CREATE	0
#define FW_APPEND	1
#define FW_NEWLINE	2

extern unsigned long f_size(const char *path);
extern int f_exists(const char *file);
extern int f_read(const char *file, void *buffer, int max);												// returns bytes read
extern int f_write(const char *file, const void *buffer, int len, unsigned flags, unsigned cmode);		//
extern int f_read_string(const char *file, char *buffer, int max);										// returns bytes read, not including term; max includes term
extern int f_write_string(const char *file, const char *buffer, unsigned flags, unsigned cmode);		//
extern int f_read_alloc(const char *path, char **buffer, int max);
extern int f_read_alloc_string(const char *path, char **buffer, int max);
extern int f_wait_exists(const char *name, int max);
extern int f_wait_notexists(const char *name, int max);


// led.c
#define LED_WLAN			0
#define LED_DIAG			1
#define LED_WHITE			2
#define LED_AMBER			3
#define LED_DMZ				4
#define LED_AOSS			5
#define LED_BRIDGE			6
#define LED_MYSTERY			7	// (unmarked LED between wireless and bridge on WHR-G54S)
#define LED_COUNT			8

#define	LED_OFF				0
#define	LED_ON				1
#define LED_BLINK			2
#define LED_PROBE			3

extern const char *led_names[];

extern void gpio_write(uint32_t bit, int en);
extern uint32_t gpio_read(void);
extern int nvget_gpio(const char *name, int *gpio, int *inv);
extern int led(int which, int mode);


// base64.c
extern int base64_encode(unsigned char *in, char *out, int inlen);			// returns amount of out buffer used
extern int base64_decode(const char *in, unsigned char *out, int inlen);	// returns amount of out buffer used
extern int base64_encoded_len(int len);
extern int base64_decoded_len(int len);										// maximum possible, not actual


// strings.c
extern const char *find_word(const char *buffer, const char *word);
extern int remove_word(char *buffer, const char *word);

#endif
