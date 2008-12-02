#include "code_pattern.h"

#if LINKSYS_MODEL == WRT54GSV4
  #if LOCALE == USA
    #define CYBERTAN_VERSION	"v1.06.2"
    #define SERIAL_NUMBER	"005"
    #define MINOR_VERSION	""
  #else	// ETSI
    #define CYBERTAN_VERSION	"v1.06.2"
    #define SERIAL_NUMBER	"005"
    #define MINOR_VERSION	""
  #endif
#elif LINKSYS_MODEL == WRT54GS	// WRT54GSV1-3
  #if LOCALE == USA
    #define CYBERTAN_VERSION	"v4.71.3"
    #define SERIAL_NUMBER	"005"
    #define MINOR_VERSION	""
  #else	// ETSI
    #define CYBERTAN_VERSION	"v4.71.3"
    #define SERIAL_NUMBER	"005"
    #define MINOR_VERSION	""
  #endif
#elif LINKSYS_MODEL == WRT54GL
  #if LOCALE == USA
    #define CYBERTAN_VERSION	"v4.30.11"
    #define SERIAL_NUMBER	"011"
    #define MINOR_VERSION	""
  #else	// ETSI
    #define CYBERTAN_VERSION	"v4.30.11"
    #define SERIAL_NUMBER	"011"
    #define MINOR_VERSION	""
  #endif
#else	// WRT54GV1-4
  #if LOCALE == USA
    #define CYBERTAN_VERSION	"v4.21.2"
    #define SERIAL_NUMBER	"000"
    #define MINOR_VERSION	""
  #else	// ETSI
    #define CYBERTAN_VERSION	"v4.21.2"
    #define SERIAL_NUMBER	"000"
    #define MINOR_VERSION	""
  #endif
#endif

#define LANG_VERSION		"v1.00.00"    //for lang.bin version control(setupwizard)
#define PMON_BOOT_VERSION	"v1.8"
#define CFE_BOOT_VERSION	"v2.4"	// for old cfe, no used now
#define CFES_BOOT_VERSION	"v3.7"

#define BOOT_IPADDR "192.168.1.1"
#define BOOT_NETMASK "255.255.255.0"

#define SUPPORT_4712_CHIP	0x0001
#define SUPPORT_INTEL_FLASH	0x0002
#define SUPPORT_5325E_SWITCH	0x0004
#define SUPPORT_4704_CHIP	0x0008
#define SUPPORT_5352E_CHIP	0x0010

struct code_header {
	char magic[4];
	char res1[4];	// for extra magic
	char fwdate[3];
	char fwvern[3];
	char id[4];	// U2ND
	char hw_ver;    // 0) for 4702, 1) for 4712, 2) for 4712L, 3) for 4704, 4) for 5352E
	char res2;
	unsigned short flags;
	unsigned char res3[10];
} ;

//#ifdef MULTILANG_SUPPORT
struct lang_header {
        char magic[4];
        char res1[4];   // for extra magic
        char fwdate[3];
        char fwvern[3];
        char id[4];     // U2ND
        char hw_ver;    // 0: for 4702, 1: for 4712
	char res2;
        unsigned long len;
        unsigned char res3[8];
} ;
//#endif

struct boot_header {
	char magic[3];
	char res[29];
};

/***************************************
 * define upnp misc                    *
 ***************************************/
#if OEM == LINKSYS
  #if LANGUAGE == ENGLISH
    #define URL			"http://www.linksys.com/"
  #else
    #define URL			"http://www.linksys.co.jp/"
  #endif
  #define DEV_FRIENDLY_NAME	MODEL_NAME
  #define DEV_MFR		"Linksys Inc."
  #define DEV_MFR_URL		URL
  #define DEV_MODEL_DESCRIPTION	"Internet Access Server"
  #define DEV_MODEL		MODEL_NAME
  #define DEV_MODEL_NO		CYBERTAN_VERSION
  #define DEV_MODEL_URL		URL
#elif OEM == PCI
  #if LANGUAGE == ENGLISH
    #define URL			"http://www.planex.com/"
  #else
    #define URL			"http://www.planex.co.jp/"
  #endif
  #define DEV_FRIENDLY_NAME	"BLW-04G Wireless Broadband Router"
  #define DEV_MFR		"Planex Communciations Inc."
  #define DEV_MFR_URL		URL
  #define DEV_MODEL_DESCRIPTION	"Internet Gateway Device with UPnP support"
  #define DEV_MODEL		"BLW-04G"
  #define DEV_MODEL_NO 		CYBERTAN_VERSION
  #define DEV_MODEL_URL		URL
#else
  #define URL			""
  #define DEV_FRIENDLY_NAME	MODEL_NAME
  #define DEV_MFR		""
  #define DEV_MFR_URL		URL
  #define DEV_MODEL_DESCRIPTION	"Internet Access Server"
  #define DEV_MODEL		MODEL_NAME
  #define DEV_MODEL_NO		CYBERTAN_VERSION
  #define DEV_MODEL_URL		URL
#endif

/***************************************
 * define Parental Control link        *
 ***************************************/
#if LOCALE == EUROPE
  #define	SIGN_UP_URL	"http://pcsvc.ourlinksys.com/eu/language.jsp"
  #define	MORE_INFO_URL	"http://www.linksys.com/pcsvc/eu/info_eu.asp"
  #define	ADMIN_URL	"http://pcsvc.ourlinksys.com/en"
#elif LOCALE == GERMANY
  #define	SIGN_UP_URL	"http://pcsvc.ourlinksys.com/de/trial.asp"
  #define	MORE_INFO_URL	"http://www.linksys.com/pcsvc/de/info_de.asp"
  #define	ADMIN_URL	"http://pcsvc.ourlinksys.com/de/admin.asp"
#elif LOCALE == FRANCE
  #define	SIGN_UP_URL	"http://pcsvc.ourlinksys.com/fr/trial.asp"
  #define	MORE_INFO_URL	"http://www.linksys.com/pcsvc/fr/info_fr.asp"
  #define	ADMIN_URL	"http://pcsvc.ourlinksys.com/fr/admin.asp"
#else
  #define	SIGN_UP_URL	"http://pcsvc.ourlinksys.com/us/trial.asp"
  #define	MORE_INFO_URL	"http://www.linksys.com/pcsvc/info.asp"
  #define	ADMIN_URL	"http://pcsvc.ourlinksys.com/us/admin.asp"
#endif

/***************************************
 * define PPTP info		       * 
 ***************************************/
#if OEM == LINKSYS
#define	PPTP_VENDOR	"Linksys"
#else
#define	PPTP_VENDOR	CT_VENDOR
#endif
#define PPTP_HOSTNAME	""

/***************************************
 * define L2TP info		       *
 ***************************************/
#if OEM == LINKSYS
#define	L2TP_VENDOR	"Linksys"
#else
#define	L2TP_VENDOR	CT_VENDOR
#endif
#define L2TP_HOSTNAME	MODEL_NAME

