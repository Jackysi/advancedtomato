#include <httpd.h>
#include <typedefs.h>
#include <bcmnvram.h>
#include <bcmutils.h>
#include <shutils.h>
#include <code_pattern.h>
#include <cy_conf.h>
#include <utils.h>
#include <syslog.h>

#include <bcmcvar.h> //Added by Daniel(2004-07-29) for EZC

#define sys_restart() kill(1, SIGHUP)
#define sys_reboot() kill(1, SIGTERM)
#define sys_stats(url) eval("stats", (url))
#define ARGV(args...) ((char *[]) { args, NULL })
#define STRUCT_LEN(name)    sizeof(name)/sizeof(name[0])
#define GOZILA_GET(name)	gozila_action ? websGetVar(wp, name, NULL) : nvram_safe_get(name);

#define SWAP(AA,BB)  { \
	char *CC; \
	CC = AA; \
	AA = BB; \
	BB = CC; \
}

/* for dhcp */
#define MAX_LEASES 254

/* for filter */
#define FILTER_IP_NUM 5
#define FILTER_PORT_NUM 5
#define FILTER_MAC_NUM 10
#define FILTER_MAC_PAGE 5
#define BLOCKED_SERVICE_NUM 2

/* for forward */
#define FORWARDING_NUM 10
#define SPECIAL_FORWARDING_NUM 10
#define UPNP_FORWARDING_NUM 15
#define PORT_TRIGGER_NUM 10
#ifdef ALG_FORWARD_SUPPORT
#define ALG_FORWARDING_NUM 15
#endif

/* for static route */
#define STATIC_ROUTE_PAGE 20

/* for wireless */
#define WL_FILTER_MAC_PAGE 2
#define WL_FILTER_MAC_NUM 20
//#define WL_FILTER_MAC_COUNT 32

#define MAC_LEN 17
#define TMP_PASSWD "d6nw5v1x2pc7st9m"

#define USE_LAN	1
#define USE_WAN 2

extern int gozila_action;
extern int error_value;
extern int debug_value;
extern int filter_id;
extern int generate_key;
extern int clone_wan_mac;
extern char http_client_ip[20];
extern int lan_ip_changed;

#ifdef HSIAB_SUPPORT
extern struct register_deviceResponse RegDevReply;
extern struct Sessioninfo *siptr;
extern struct deviceinfo di;
extern int register_status;
extern int new_device;
#endif
//Masked by Daniel(2004-07-29) for EZC
/*
struct variable {
	char *name;
	char *longname;
	void (*validate)(webs_t wp, char *value, struct variable *v);
	char **argv;
	int nullok;
};
*/
struct onload {
        char *name;
        int (*go)(webs_t wp, char *arg);
};

struct lease_t {
	unsigned char chaddr[16];
	u_int32_t yiaddr;
	u_int32_t expires;
	char hostname[64];
};

struct apply_action {
	char *name;
	char *service;
	int sleep_time;
	int action;
        int (*go)(webs_t wp);
};

struct gozila_action {
	char *name;
	char *type;
	char *service;
	int sleep_time;
	int action;
        int (*go)(webs_t wp);
};

enum {SET, GET};

enum {	// return code
	START_FROM = 10,
#ifdef HSIAB_SUPPORT
	DISABLE_HSIAB,
	RESTART_HSIAB,
#endif
#ifdef SAMBA_SUPPORT
	SAMBA_FORK,
#endif
     };

/* for index */
extern int ej_show_index_setting(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_compile_date(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_compile_time(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_get_wl_max_channel(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_get_wl_domain(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_get_clone_mac(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_show_wan_domain(int eid, webs_t wp, int argc, char_t **argv);
extern void validate_ntp(webs_t wp, char *value, struct variable *v);
extern void validate_lan_ipaddr(webs_t wp, char *value, struct variable *v);
extern void validate_wan_ipaddr(webs_t wp, char *value, struct variable *v);
extern int clone_mac(webs_t wp);

/* for status */
extern int ej_show_status(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_localtime(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_nvram_status_get(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_dhcp_remaining_time(int eid, webs_t wp, int argc, char_t **argv);
extern int dhcp_renew(webs_t wp);
extern int dhcp_release(webs_t wp);
extern int stop_ppp(webs_t wp);
extern int stop_ppp_1(webs_t wp);
extern int ej_show_status_setting(int eid, webs_t wp, int argc, char_t **argv);

#ifdef ARP_TABLE_SUPPORT
extern int ej_dump_arp_table(int eid, webs_t wp, int argc, char_t **argv);
#endif

#ifdef DNS_SUPPORT
extern int ej_dump_dns_entry(int eid, webs_t wp, int argc, char_t **argv);
extern int apply_dns(webs_t wp);
#endif

#ifdef SES_BUTTON_SUPPORT
extern int set_ses_long_push(webs_t wp);
extern int set_ses_short_push(webs_t wp);
extern int reset_ses(webs_t wp);
extern int ej_get_ses_status(int eid, webs_t wp, int argc, char_t **argv);
#endif


#ifdef TINYLOGIN_SUPPORT
extern int del_user(webs_t wp);
extern int ej_dump_account(int eid, webs_t wp, int argc, char_t **argv);
extern int apply_accounts(webs_t wp);
extern int set_tinylogin_info();
#endif

#ifdef WAKE_ON_LAN_SUPPORT
extern int ej_get_wol_mac(int eid, webs_t wp, int argc, char_t **argv);
extern int wol_wakeup(webs_t wp); 
#endif

#ifdef SAMBA_SUPPORT
extern int ej_dumpsamba(int eid, webs_t wp, int argc, char_t **argv);
extern void do_samba_file(char *url, webs_t wp, int len, char *boundary);
extern void do_samba_cgi(char *url, webs_t wp);
extern int smb_getshare(webs_t wp);
extern int smb_getdir(webs_t wp);  
extern int smb_getfile(webs_t wp); 
#endif

/*for dhcp */
extern int ej_dumpleases(int eid, webs_t wp, int argc, char_t **argv);
extern void validate_dhcp(webs_t wp, char *value, struct variable *v);
extern void dhcp_check(webs_t wp, char *value, struct variable *v);
extern int delete_leases(webs_t wp);

/* for log */
extern int ej_dumplog(int eid, webs_t wp, int argc, char_t **argv);
#ifdef SYSLOG_SUPPORT
extern int ej_dump_syslog(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_dump_log_settings(int eid, webs_t wp, int argc, char_t **argv);
extern void validate_log_settings(webs_t wp, char *value, struct variable *v);
extern int set_log_type(webs_t wp);
#endif

extern int log_onload(webs_t wp);
extern int filtersummary_onload(webs_t wp, char *arg);

/* for upgrade */
extern void do_upgrade_post(char *url, webs_t stream, int len, char *boundary);
extern void do_upgrade_cgi(char *url,  webs_t stream);

extern int sys_restore(char *url, webs_t stream, int *total);
extern void do_restore_post(char *url, webs_t stream, int len, char *boundary);
extern void do_restore_cgi(char *url,  webs_t stream);

extern int macclone_onload(webs_t wp, char *arg);
#ifdef DDM_SUPPORT
/* for DDM */
extern void do_ddm_post(char *url, webs_t stream, int len, char *boundary);
extern int ej_ddm_dumpleases(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_ddm_check_passwd(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_ddm_error_no(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_ddm_error_desc(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_ddm_show_ipend(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_ddm_show_wanproto(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_ddm_show_lanproto(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_ddm_show_idletime(int eid, webs_t wp, int argc, char_t **argv);
#endif
#ifdef MULTIPLE_LOGIN_SUPPORT //r
extern int ej_user_account_get(int eid, webs_t wp, int argc, char_t **argv);
extern int save_user_account(webs_t wp);
extern int validate_user_account(webs_t wp);
extern int ej_http_user_name_get(int eid, webs_t wp, int argc, char_t **argv);
#endif
extern int ej_http_name_get(int eid, webs_t wp, int argc, char_t **argv);
/* for filter */
extern int ej_filter_init(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_filter_summary_show(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_filter_ip_get(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_filter_port_get(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_filter_dport_get(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_filter_mac_get(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_filter_policy_select(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_filter_policy_get(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_filter_tod_get(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_filter_web_get(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_filter_port_services_get(int eid, webs_t wp, int argc, char_t **argv);
extern void validate_filter_policy(webs_t wp, char *value, struct variable *v);
extern void validate_filter_ip_grp(webs_t wp, char *value, struct variable *v);
extern void validate_filter_mac_grp(webs_t wp, char *value, struct variable *v);
extern void validate_filter_dport_grp(webs_t wp, char *value, struct variable *v);
extern void validate_filter_port(webs_t wp, char *value, struct variable *v);
extern void validate_filter_web(webs_t wp, char *value, struct variable *v);
extern void validate_blocked_service(webs_t wp, char *value, struct variable *v);
extern int filter_onload(webs_t wp);
extern int save_policy(webs_t wp);
extern int summary_delete_policy(webs_t wp);
extern int single_delete_policy(webs_t wp);
extern int save_services_port(webs_t wp);

/* for forward */
extern int ej_port_forward_table(int eid, webs_t wp, int argc, char_t **argv);
extern void validate_forward_proto(webs_t wp, char *value, struct variable *v);
#ifdef UPNP_FORWARD_SUPPORT
extern int ej_forward_upnp(int eid, webs_t wp, int argc, char_t **argv);
extern void validate_forward_upnp(webs_t wp, char *value, struct variable *v);
#endif
#ifdef ALG_FORWARD_SUPPORT
extern int ej_forward_alg(int eid, webs_t wp, int argc, char_t **argv);
extern void validate_forward_alg(webs_t wp, char *value, struct variable *v);
#endif
#ifdef SPECIAL_FORWARD_SUPPORT
extern int ej_spec_forward_table(int eid, webs_t wp, int argc, char_t **argv);
extern void validate_forward_spec(webs_t wp, char *value, struct variable *v);
#endif
#ifdef PORT_TRIGGER_SUPPORT
extern int ej_port_trigger_table(int eid, webs_t wp, int argc, char_t **argv);
extern void validate_port_trigger(webs_t wp, char *value, struct variable *v);
#endif

/* for dynamic route */
extern int ej_dump_route_table(int eid, webs_t wp, int argc, char_t **argv);
extern void validate_dynamic_route(webs_t wp, char *value, struct variable *v);
extern int dynamic_route_onload(webs_t wp);

/* for static route */
extern int ej_static_route_setting(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_static_route_table(int eid, webs_t wp, int argc, char_t **argv);
extern void validate_static_route(webs_t wp, char *value, struct variable *v);
extern int delete_static_route(webs_t wp);

/* for wireless */
extern void validate_wl_key(webs_t wp, char *value, struct variable *v);
extern void validate_wl_wep(webs_t wp, char *value, struct variable *v);
extern void validate_wl_auth(webs_t wp, char *value, struct variable *v);
extern void validate_d11_channel(webs_t wp, char *value, struct variable *v);
extern int add_active_mac(webs_t wp);
extern int wl_active_onload(webs_t wp, char *arg);
extern int generate_key_64(webs_t wp);
extern int generate_key_128(webs_t wp);
extern int ej_get_wep_value(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_get_wl_active_mac(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_get_wl_value(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_show_wpa_setting(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_show_wpa_setting2(int eid, webs_t wp, int argc, char_t **argv);
extern void wl_unit(webs_t wp, char *value, struct variable *v);
extern void validate_wpa_psk(webs_t wp, char *value, struct variable *v);
extern void validate_wl_auth_mode(webs_t wp, char *value, struct variable *v);
extern void validate_security_mode(webs_t wp, char *value, struct variable *v);
extern void validate_security_mode2(webs_t wp, char *value, struct variable *v);
extern int ej_wl_ioctl(int eid, webs_t wp, int argc, char_t **argv);
extern void validate_wl_gmode(webs_t wp, char *value, struct variable *v);
extern void validate_wl_net_mode(webs_t wp, char *value, struct variable *v);
extern void convert_wl_gmode(char *value);

/* for ddns */
extern int ddns_save_value(webs_t wp);
extern int ddns_update_value(webs_t wp);

extern void validate_macmode(webs_t wp, char *value, struct variable *v);
extern void validate_wl_hwaddrs(webs_t wp, char *value, struct variable *v);
extern int ej_wireless_active_table(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_wireless_filter_table(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_show_wl_wep_setting(int eid, webs_t wp, int argc, char_t **argv);
extern void validate_wl_wep_key(webs_t wp, char *value, struct variable *v);

/* for ddns */
extern int ej_show_ddns_status(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_show_ddns_ip(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_show_ddns_setting(int eid, webs_t wp, int argc, char_t **argv);
//
/* for test */
extern int ej_wl_packet_get(int eid, webs_t wp, int argc, char_t **argv);
extern int Change_Ant(char *value);

#ifdef AOL_SUPPORT
/* for aol */
extern int ej_aol_value_get(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_aol_settings_show(int eid, webs_t wp, int argc, char_t **argv);
#endif

#ifdef EMI_TEST
extern int emi_test_exec(webs_t wp);
extern int emi_test_del(webs_t wp);
extern int ej_dump_emi_test_log(int eid, webs_t wp, int argc, char_t **argv);
#endif

#ifdef DIAG_SUPPORT
extern int ej_dump_ping_log(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_dump_traceroute_log(int eid, webs_t wp, int argc, char_t **argv);
extern int diag_ping_start(webs_t wp);
extern int diag_ping_stop(webs_t wp);
extern int diag_ping_clear(webs_t wp);
extern int diag_traceroute_start(webs_t wp);
extern int diag_traceroute_stop(webs_t wp);
extern int diag_traceroute_clear(webs_t wp);
extern int ping_onload(webs_t wp, char *arg);
extern int traceroute_onload(webs_t wp, char *arg);
#endif

#ifdef HSIAB_SUPPORT
extern int ej_get_hsiab_value(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_show_hsiab_setting(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_show_hsiab_config(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_dump_hsiab_db(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_dump_hsiab_msg(int eid, webs_t wp, int argc, char_t **argv);
extern int hsiab_register(webs_t wp);
extern int hsiab_register_ok(webs_t wp);
extern int hsiab_finish_registration(webs_t wp);
#endif

/* for verizon */
#ifdef VERIZON_LAN_SUPPORT
extern void validate_lan_proto(webs_t wp, char *value, struct variable *v);
extern void validate_dhcrelay_ipaddr(webs_t wp, char *value, struct variable *v);
extern int ej_show_dhcp_setting(int eid, webs_t wp, int argc, char_t **argv);
#endif
#ifdef UDHCPD_STATIC_SUPPORT
extern void validate_dhcp_statics(webs_t wp, char *value, struct variable *v);
extern int ej_dhcp_static_get(int eid, webs_t wp, int argc, char_t **argv);
#endif
#ifdef DHCP_FILTER_SUPPORT
extern int ej_DHCP_filter_MAC_table(int eid, webs_t wp, int argc, char_t **argv);
extern char *DHCP_filter_mac_get(char *type, int which);
extern void validate_DHCP_hwaddrs(webs_t wp, char *value, struct variable *v);
#endif



/* for all */
extern int ej_onload(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_get_web_page_name(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_compile_date(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_compile_time(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_get_model_name(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_get_firmware_version(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_get_firmware_title(int eid, webs_t wp, int argc, char_t **argv);

extern int valid_ipaddr(webs_t wp, char *value, struct variable *v);
extern int valid_range(webs_t wp, char *value, struct variable *v);
extern int valid_hwaddr(webs_t wp, char *value, struct variable *v);
extern int valid_choice(webs_t wp, char *value, struct variable *v);
extern int valid_netmask(webs_t wp, char *value, struct variable *v);
extern int valid_name(webs_t wp, char *value, struct variable *v);
extern int valid_merge_ipaddrs(webs_t wp, char *value, struct variable *v);
extern int valid_wep_key(webs_t wp, char *value, struct variable *v);
extern void validate_choice(webs_t wp, char *value, struct variable *v);

#ifdef GOOGLE_SUPPORT
extern void validate_google(webs_t wp, char *value, struct variable *v);
#endif

extern int get_dns_ip(char *name, int which, int count);
extern int get_single_ip(char *ipaddr, int which);
extern int get_merge_ipaddr(char *name, char *ipaddr);
extern int get_merge_mac(char *name, char *macaddr);
extern char *rfctime(const time_t *timep);
extern int legal_ipaddr(char *value);
extern int legal_hwaddr(char *value);
extern int legal_netmask(char *value);
extern int legal_ip_netmask(char *sip, char *smask, char *dip);
extern int find_pattern(const char *data, size_t dlen,
                        const char *pattern, size_t plen,
                        char term, 
                        unsigned int *numoff,
                        unsigned int *numlen);

extern int find_match_pattern(char *name, size_t mlen,
                        const char *data,
                        const char *pattern,
                        char *def);

extern int find_each(char *name, int len,
	          char *data,
	          char *token,
	          int which,
	          char *def);

/* 
 * set type to 1 to replace ' ' with "&nbsp;" and ':' with "&semi;"
 * set type to 2 to replace "&nbsp;" with ' ' and "&semi;" with ':'
 */
extern int filter_name(char *old_name, char *new_name, size_t size, int type);

/* check the value for a digit (0 through 9) 
 * set flag to 0 to ignore zero-length values
 */
extern int ISDIGIT(char *value, int flag);

/* checks  whether  value  is  a  7-bit unsigned char value that fits into the ASCII character set 
 * set flag to 0 to ignore zero-length values
 */
extern int ISASCII(char *value, int flag);

extern void do_setup_wizard(char *url, webs_t stream);

extern int ej_show_logo(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_show_sysinfo(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_show_miscinfo(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_get_backup_name(int eid, webs_t wp, int argc, char_t **argv);

extern struct servent * my_getservbyport(int port, const char *proto);
extern int get_single_mac(char *macaddr, int which);

extern int StopContinueTx(char *value);
extern int StartContinueTx(char *value);
extern int Check_TSSI(char *value);
extern int Get_TSSI(char *value);
extern int Enable_TSSI(char *value);
extern int Satrt_EPI(char *value);

extern void LOG(int level, const char *fmt,...);

extern char *num_to_protocol(int num);
extern int protocol_to_num(char *proto);
#ifdef BACKUP_RESTORE_SUPPORT
extern  void do_backup(char *path, webs_t stream);
extern int ej_view_config(int eid, webs_t wp, int argc, char_t **argv);
#endif

#ifdef HW_QOS_SUPPORT
extern int ej_per_port_option(int eid, webs_t wp, int argc, char_t **argv);
extern void validate_port_qos(webs_t wp, char *value, struct variable *v);
#if 1 //Added by crazy 20070717 - Fixed issue id 7684, 7693
	/*
	   After clicked the Save Settings button on webpage 
	   'Qos.asp' some times, the DUT will crash when testing 
	   throughput. 
	*/
extern void validate_ip_forward(webs_t wp, char *value, struct variable *v);
#endif
#endif

#ifdef PERFORMANCE_SUPPORT
extern int ej_perform(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_show_wl_status(int eid, webs_t wp, int argc, char_t **argv);
#endif

extern int ej_dump_site_suvery(int eid, webs_t wp, int argc, char_t **argv);

extern int ej_get_url(int eid, webs_t wp, int argc, char_t **argv);

extern int ej_wme_match_op(int eid, webs_t wp, int argc, char_t **argv);
extern void validate_noack(webs_t wp, char *value, struct variable *v);
extern void validate_wl_wme_params(webs_t wp, char *value, struct variable *v);
extern void validate_wl_preauth(webs_t wp, char *value, struct variable *v);
extern void validate_wl_akm(webs_t wp, char *value, struct variable *v);

#ifdef WCN_SUPPORT
extern void do_wcn_post(char *url, webs_t stream, int len, char *boundary);
extern void do_wcn_cgi(char *url,  webs_t stream);
extern int ej_get_wcn_output_file_name(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_get_wcn_output_file_path_name(int eid, webs_t wp, int argc, char_t **argv);
#endif

#ifdef INTEL_VIIV_SUPPORT
extern void do_viiv_cgi(char *url, webs_t stream);
extern void do_viiv_post(char *url, webs_t stream, int len, char *boundary);
extern void do_viiv_html(char *url, webs_t stream);
extern void do_viiv_xml(char *url, webs_t stream);
extern void do_viiv_general_xml(char *url, webs_t stream);
extern void do_router_info(char *url, webs_t stream);
#endif

extern int _websWrite(webs_t wp, char *value);
