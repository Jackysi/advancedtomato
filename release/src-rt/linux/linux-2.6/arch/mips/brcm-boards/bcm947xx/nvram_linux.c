/*
 * NVRAM variable manipulation (Linux kernel half)
 *
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: nvram_linux.c,v 1.8 2008/07/04 01:15:09 Exp $
 */

#include <linux/config.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/bootmem.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mtd/mtd.h>
//#include <mtd/mtd-user.h>
#include <asm/addrspace.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/cacheflush.h>

#include <typedefs.h>
#include <bcmendian.h>
#include <bcmnvram.h>
#include <bcmutils.h>
#include <bcmdefs.h>
#include <hndsoc.h>
#include <siutils.h>
#include <hndmips.h>
#include <sflash.h>

/* In BSS to minimize text size and page aligned so it can be mmap()-ed */
static char nvram_buf[NVRAM_SPACE] __attribute__((aligned(PAGE_SIZE)));

#define CFE_UPDATE 1 // added by Chen-I for mac/regulation update
#ifdef CFE_UPDATE
//#include <sbextif.h>

extern void bcm947xx_watchdog_disable(void);

#define CFE_SPACE       256*1024
#define CFE_NVRAM_START 0x00000
#define CFE_NVRAM_END   0x01fff
#define CFE_NVRAM_SPACE 64*1024
static struct mtd_info *cfe_mtd = NULL;
static char *CFE_NVRAM_PREFIX="asuscfe";
static char *CFE_NVRAM_COMMIT="asuscfecommit";
static char *CFE_NVRAM_WATCHDOG="asuscfewatchdog";
char *cfe_buf;// = NULL;
struct nvram_header *cfe_nvram_header; // = NULL;

static u_int32_t cfe_offset;
static u_int32_t cfe_embedded_size;
static int get_embedded_block(struct mtd_info *mtd, char *buf, size_t erasesize,
                       u_int32_t *offset, struct nvram_header **header, u_int32_t *emb_size);

static int cfe_init(void);
static int cfe_update(const char *keyword, const char *value);
static int cfe_dump(void);
static int cfe_commit(void);
#endif


#define WPS	1

#ifdef MODULE

#define early_nvram_get(name) nvram_get(name)

#else /* !MODULE */

/* Global SB handle */
extern void *bcm947xx_sih;
extern spinlock_t bcm947xx_sih_lock;

/* Convenience */
#define sih bcm947xx_sih
#define sih_lock bcm947xx_sih_lock
#define KB * 1024
#define MB * 1024 * 1024

//#define NLS_XFR 1              /* added by Jiahao for WL500gP */
#ifdef NLS_XFR

#include <linux/nls.h>

static char *NLS_NVRAM_U2C="asusnlsu2c";
static char *NLS_NVRAM_C2U="asusnlsc2u";
__u16 unibuf[1024];
char codebuf[1024];
char tmpbuf[1024];

void
asusnls_u2c(char *name)
{
	char *codepage;
	char *xfrstr;
	struct nls_table *nls;
	int ret, len;

	strcpy(codebuf, name);
	codepage=codebuf+strlen(NLS_NVRAM_U2C);
	if((xfrstr=strchr(codepage, '_')))
	{
		*xfrstr=NULL;
		xfrstr++;
		/* debug message, start */
/*
		printk("%s, xfr from utf8 to %s\n", xfrstr, codepage);
		int j;
		printk("utf8:    %d, ", strlen(xfrstr));
		for(j=0;j<strlen(xfrstr);j++)
			printk("%X ", (unsigned char)xfrstr[j]);
		printk("\n");
*/
		/* debug message, end */

		nls=load_nls(codepage);
		if(!nls)
		{
			printk("NLS table is null!!\n");
		}
		else {
			len = 0;
			if (ret=utf8_mbstowcs(unibuf, xfrstr, strlen(xfrstr)))
			{
				int i;
				for (i = 0; (i < ret) && unibuf[i]; i++) {
					int charlen;
					charlen = nls->uni2char(unibuf[i], &name[len], NLS_MAX_CHARSET_SIZE);
					if (charlen > 0) {
						len += charlen;
					}
					else {
						//name[len++] = '?';
						strcpy(name, "");
						unload_nls(nls);
						return;
					}
				}
				name[len] = 0;
			}
			unload_nls(nls);
			/* debug message, start */
/*
			int i;
			printk("unicode: %d, ", ret);
			for (i=0;i<ret;i++)
				printk("%X ", unibuf[i]);
			printk("\n");
			printk("local:   %d, ", strlen(name));
			for (i=0;i<strlen(name);i++)
				printk("%X ", (unsigned char)name[i]);
			printk("\n");
			printk("local:   %s\n", name);
*/
			/* debug message, end */

			if(!len)
			{
				printk("can not xfr from utf8 to %s\n", codepage);
				strcpy(name, "");
			}
		}
	}
	else
	{
		strcpy(name, "");
	}
}

void                                                                                                                         
asusnls_c2u(char *name)
{
	char *codepage;
	char *xfrstr;
	struct nls_table *nls;
	int ret;

	strcpy(codebuf, name);
	codepage=codebuf+strlen(NLS_NVRAM_C2U);
	if((xfrstr=strchr(codepage, '_')))
	{
		*xfrstr=NULL;
		xfrstr++;

		/* debug message, start */
/*
		printk("%s, xfr from %s to utf8\n", xfrstr, codepage);
		printk("local:   %d, ", strlen(xfrstr));
		int j;
		for (j=0;j<strlen(xfrstr);j++)
			printk("%X ", (unsigned char)xfrstr[j]);
		printk("\n");
		printk("local:   %s\n", xfrstr);
*/
		/* debug message, end */

		strcpy(name, "");
		nls=load_nls(codepage);
		if(!nls)
		{
			printk("NLS table is null!!\n");
		}
		else
		{
			int charlen;
			int i;
			int len = strlen(xfrstr);
			for (i = 0; len && *xfrstr; i++, xfrstr += charlen, len -= charlen) {   /* string to unicode */
				charlen = nls->char2uni(xfrstr, len, &unibuf[i]);
				if (charlen < 1) {
					//unibuf[i] = 0x003f;     /* a question mark */
					//charlen = 1;
					strcpy(name ,"");
					unload_nls(nls);
					return;
				}
			}
			unibuf[i] = 0;
			ret=utf8_wcstombs(name, unibuf, 1024);  /* unicode to utf-8, 1024 is size of array unibuf */
			name[ret]=0;
			unload_nls(nls);
			/* debug message, start */
/*
			int k;
			printk("unicode: %d, ", i);
			for(k=0;k<i;k++)
				printk("%X ", unibuf[k]);
			printk("\n");
			printk("utf-8:    %s, %d, ", name, strlen(name));
			for (i=0;i<strlen(name);i++)
				printk("%X ", (unsigned char)name[i]);
			printk("\n");
*/
			/* debug message, end */
			if(!ret)
			{
				printk("can not xfr from %s to utf8\n", codepage);
				strcpy(name, "");
			}
		}
	}
	else
	{
		strcpy(name, "");
	}
}

/* Jiahao */
static int
nvram_xfr(char *buf)
{
	char *name = tmpbuf;
	ssize_t ret=0;

	printk("nvram xfr 1: %s\n", buf);
	if (copy_from_user(name, buf, strlen(buf)+1)) {
		ret = -EFAULT;
		goto done;
	}

	if (strncmp(tmpbuf, NLS_NVRAM_U2C, strlen(NLS_NVRAM_U2C))==0)
	{
		asusnls_u2c(tmpbuf);
	}
	else if (strncmp(buf, NLS_NVRAM_C2U, strlen(NLS_NVRAM_C2U))==0)
	{
		asusnls_c2u(tmpbuf);
	}
	else
	{
		strcpy(tmpbuf, "");
		printk("nvram xfr 2: %s\n", tmpbuf);
	}

	if (copy_to_user(buf, tmpbuf, strlen(tmpbuf)+1))
	{
		ret = -EFAULT;
		goto done;
	}
	printk("nvram xfr 3: %s\n", tmpbuf);

done:
	return ret;
}

#endif

/* Probe for NVRAM header */
static int
early_nvram_init(void)
{
	struct nvram_header *header;
	chipcregs_t *cc;
	struct sflash *info = NULL;
	int i;
	uint32 base, off, lim;
	u32 *src, *dst;

	if ((cc = si_setcore(sih, CC_CORE_ID, 0)) != NULL) {
		base = KSEG1ADDR(SI_FLASH2);
		switch (readl(&cc->capabilities) & CC_CAP_FLASH_MASK) {
		case PFLASH:
			lim = SI_FLASH2_SZ;
			break;

		case SFLASH_ST:
		case SFLASH_AT:
			if ((info = sflash_init(sih, cc)) == NULL)
				return -1;
			lim = info->size;
			break;

		case FLASH_NONE:
		default:
			return -1;
		}
	} else {
		/* extif assumed, Stop at 4 MB */
		base = KSEG1ADDR(SI_FLASH1);
		lim = SI_FLASH1_SZ;
	}

	off = FLASH_MIN;
	while (off <= lim) {
		/* Windowed flash access */
		header = (struct nvram_header *) KSEG1ADDR(base + off - NVRAM_SPACE);
		if (header->magic == NVRAM_MAGIC)
			if (nvram_calc_crc(header) == (uint8) header->crc_ver_init) {
				goto found;
			}
		off <<= 1;
	}

	/* Try embedded NVRAM at 4 KB and 1 KB as last resorts */
	header = (struct nvram_header *) KSEG1ADDR(base + 4 KB);
	if (header->magic == NVRAM_MAGIC)
		if (nvram_calc_crc(header) == (uint8) header->crc_ver_init) {
			goto found;
		}

	header = (struct nvram_header *) KSEG1ADDR(base + 1 KB);
	if (header->magic == NVRAM_MAGIC)
		if (nvram_calc_crc(header) == (uint8) header->crc_ver_init) {
			goto found;
		}

	return -1;

found:
	src = (u32 *) header;
	dst = (u32 *) nvram_buf;
	for (i = 0; i < sizeof(struct nvram_header); i += 4)
		*dst++ = *src++;
	for (; i < header->len && i < NVRAM_SPACE; i += 4)
		*dst++ = ltoh32(*src++);

	return 0;
}

/* Early (before mm or mtd) read-only access to NVRAM */
static char *
early_nvram_get(const char *name)
{
	char *var, *value, *end, *eq;

	if (!name)
		return NULL;

	/* Too early? */
	if (sih == NULL)
		return NULL;

	if (!nvram_buf[0])
		if (early_nvram_init() != 0) {
			printk("early_nvram_get: Failed reading nvram var %s\n", name);
			return NULL;
		}

	/* Look for name=value and return value */
	var = &nvram_buf[sizeof(struct nvram_header)];
	end = nvram_buf + sizeof(nvram_buf) - 2;
	end[0] = end[1] = '\0';
	for (; *var; var = value + strlen(value) + 1) {
		if (!(eq = strchr(var, '=')))
			break;
		value = eq + 1;
		if ((eq - var) == strlen(name) && strncmp(var, name, (eq - var)) == 0)
			return value;
	}

	return NULL;
}

static int
early_nvram_getall(char *buf, int count)
{
	char *var, *end;
	int len = 0;

	/* Too early? */
	if (sih == NULL)
		return -1;

	if (!nvram_buf[0])
		if (early_nvram_init() != 0) {
			printk("early_nvram_getall: Failed reading nvram var\n");
			return -1;
		}

	bzero(buf, count);

	/* Write name=value\0 ... \0\0 */
	var = &nvram_buf[sizeof(struct nvram_header)];
	end = nvram_buf + sizeof(nvram_buf) - 2;
	end[0] = end[1] = '\0';
	for (; *var; var += strlen(var) + 1) {
		if ((count - len) <= (strlen(var) + 1))
			break;
		len += sprintf(buf + len, "%s", var) + 1;
	}

	return 0;
}
#endif /* !MODULE */

extern char * _nvram_get(const char *name);
extern int _nvram_set(const char *name, const char *value);
extern int _nvram_unset(const char *name);
extern int _nvram_getall(char *buf, int count);
extern int _nvram_commit(struct nvram_header *header);
extern int _nvram_init(void *sih);
extern void _nvram_exit(void);

/* Globals */
static spinlock_t nvram_lock = SPIN_LOCK_UNLOCKED;
static struct semaphore nvram_sem;
static unsigned long nvram_offset = 0;
static int nvram_major = -1;
static struct class *nvram_class = NULL;
static struct mtd_info *nvram_mtd = NULL;

int
_nvram_read(char *buf)
{
	struct nvram_header *header = (struct nvram_header *) buf;
	size_t len;

	if (!nvram_mtd ||
	    nvram_mtd->read(nvram_mtd, nvram_mtd->size - NVRAM_SPACE, NVRAM_SPACE, &len, buf) ||
	    len != NVRAM_SPACE ||
	    header->magic != NVRAM_MAGIC) {
		/* Maybe we can recover some data from early initialization */
		memcpy(buf, nvram_buf, NVRAM_SPACE);
	}

	return 0;
}

struct nvram_tuple *
_nvram_realloc(struct nvram_tuple *t, const char *name, const char *value)
{
	if ((nvram_offset + strlen(value) + 1) > NVRAM_SPACE)
		return NULL;

	if (!t) {
		if (!(t = kmalloc(sizeof(struct nvram_tuple) + strlen(name) + 1, GFP_ATOMIC)))
			return NULL;

		/* Copy name */
		t->name = (char *) &t[1];
		strcpy(t->name, name);

		t->value = NULL;
	}

	/* Copy value */
	if (!t->value || strcmp(t->value, value)) {
		t->value = &nvram_buf[nvram_offset];
		strcpy(t->value, value);
		nvram_offset += strlen(value) + 1;
	}

	return t;
}

void
_nvram_free(struct nvram_tuple *t)
{
	if (!t)
		nvram_offset = 0;
	else
		kfree(t);
}

int
nvram_init(void *sih)
{
	return 0;
}

int
nvram_set(const char *name, const char *value)
{
	unsigned long flags;
	int ret;
	struct nvram_header *header;
	char wps_name[32];
	int wep_len;

	spin_lock_irqsave(&nvram_lock, flags);

	//printk("nvram_set: name = %s, value = %s!\n", name, value);
#ifdef CFE_UPDATE //write back to default sector as well, Chen-I
        if(strncmp(name, CFE_NVRAM_PREFIX, strlen(CFE_NVRAM_PREFIX))==0)
        {
                if(strcmp(name, CFE_NVRAM_COMMIT)==0)
                        cfe_commit();
                else if(strcmp(name, "asuscfe_dump") == 0)
                        ret = cfe_dump();
                else if(strcmp(name, CFE_NVRAM_WATCHDOG)==0)
                {
                        bcm947xx_watchdog_disable();
                }
                else
                {
                        cfe_update(name+strlen(CFE_NVRAM_PREFIX), value);
                        _nvram_set(name+strlen(CFE_NVRAM_PREFIX), value);
                }
        }
        else
#endif

#if WPS
	if (strncmp(name, "wlx_", 4) == 0) {
		memset(wps_name, 0, sizeof(wps_name));
		sprintf(wps_name, "wl0_%s", name+4);
		ret = _nvram_set(wps_name, value);
		spin_unlock_irqrestore(&nvram_lock, flags);
                return ret;
	}

	if (strncmp(name, "wl0_", 4) == 0) 
	{
		/* Authentication mode */
		if (strncmp(name, "wl0_akm", 7) == 0) {
			if (strncmp(value, "psk2", 4) == 0) { 		// WPA2-Personal
        	                _nvram_set("wl_auth_mode", "psk");
	                        _nvram_set("wl_wpa_mode", "2");
			}
		        else if (strncmp(value, "psk ", 4) == 0) { 	// WPA-Auto-Personal
        	                _nvram_set("wl_auth_mode", "psk");
                	        _nvram_set("wl_wpa_mode", "0");
	                }
                	else if (strncmp(value, "psk", 3) == 0) { 	// WPA-Personal
        	                _nvram_set("wl_auth_mode", "psk");
                	        _nvram_set("wl_wpa_mode", "1");
	                }
        	        else if (strncmp(value, "wpa2", 4) == 0) {	// WPA2-Enterprise
                	        _nvram_set("wl_auth_mode", "wpa2");
        	        }
		        else if (strncmp(value, "wpa ", 4) == 0) {	// WPA-Auto-Enterprise
	                       	_nvram_set("wl_wpa_mode", "4");
        	                _nvram_set("wl_auth_mode", "wpa");
                	}
                	else if (strncmp(value, "wpa", 3) == 0) {	// WPA-Enterprise
	                       	_nvram_set("wl_wpa_mode", "3");
        	                _nvram_set("wl_auth_mode", "wpa");
                	}
			_nvram_set("wl_akm", value);
		}
		/* WPS KEY*/
		else if (strcmp(name, "wl0_wpa_psk") == 0) {
			_nvram_set("wl_wpa_psk", value);
		}
		/* WEP type */
#if 1
		else if ((strncmp(name, "wl0_key", 7) == 0 )) {

			wep_len = strlen (value);
			memset(wps_name, 0, sizeof(wps_name));

			if ((wep_len == 5) || (wep_len == 10)) { /* wl0_key1~4*/
//				_nvram_set ("wl0_wep_x", "1");
				_nvram_set ("wl_wep_x", "1");
				sprintf(wps_name, "wl_%s", name+4);
			}
			else if ((wep_len == 13) || (wep_len == 26)) { 
//				_nvram_set ("wl0_wep_x", "2");
				_nvram_set ("wl_wep_x", "2");
				sprintf(wps_name, "wl_%s", name+4);
			}
			else { /* wl0_key index */
				sprintf(wps_name, "wl_%s", name+4);
			}
			_nvram_set(wps_name, value);
		}
#endif
		else if (strcmp(name, "wl0_ssid") == 0) {
	//		_nvram_set("wl0_ssid2", value);
	//		_nvram_set("wl_ssid2", value);
			_nvram_set("wl_ssid", value);
		}	
		else if (strcmp(name, "wl0_crypto") == 0) {
			_nvram_set("wl_crypto", value);
		}
		else if (strncmp(name, "wl0_wps", 7) == 0) {
			memset(wps_name, 0, sizeof(wps_name));
			sprintf(wps_name, "wl_%s", name+4);
			_nvram_set(wps_name, value);
		}

	}
	else if (strncmp(name, "wps_random_ssid_prefix", 22) == 0) {
		memset(wps_name, 0, sizeof(wps_name));
		sprintf(wps_name, "ASUS_");
		ret = _nvram_set("wps_random_ssid_prefix", wps_name);
		spin_unlock_irqrestore(&nvram_lock, flags);
		return ret;
	}

	if ((strcmp(name, "wl_wps_config_state") == 0) || (strcmp(name, "wl0_wps_config_state") == 0))
		_nvram_set("wps_config_state", value);
//	else if ((strncmp(name, "wps_proc_status", 15) == 0 ) && (strcmp(value, "4"))) /* WPS success*/
//		_nvram_set("wps_config_state", "1");

#if 0
	if(!strcmp(name, "wl0_ssid"))
		_nvram_set("wl_ssid", value);
	if(!strcmp(name, "wl0_ssid"))
		_nvram_set("wl_ssid", value);
        /* Set Wireless encryption */
        if (!strcmp(name, "wl0_akm")) {
		_nvram_set("wl_auth_mode", "psk");
                _nvram_set("wl_wpa_mode", "1");
	}
	else if (!strcmp("wl0_akm", "psk2")) {
                _nvram_set("wl_auth_mode", "psk");
                _nvram_set("wl_wpa_mode", "2");
        }
        else if (nvram_match("wl0_akm", "wpa")) {
                _nvram_set("wl_auth_mode", "wpa");
                _nvram_set("wl_wpa_mode", "3");
        }
        else if (nvram_match("wl0_akm", "wpa2")) {
                _nvram_set("wl_auth_mode", "wpa2");
        }
        else
                _nvram_set("wl_auth_mode", nvram_get("wl0_auth_mode"));
#endif
#endif

	if ((ret = _nvram_set(name, value))) {
		/* Consolidate space and try again */
		if ((header = kmalloc(NVRAM_SPACE, GFP_ATOMIC))) {
			if (_nvram_commit(header) == 0)
				ret = _nvram_set(name, value);
			kfree(header);
		}
	}
	spin_unlock_irqrestore(&nvram_lock, flags);

	return ret;
}

char *
real_nvram_get(const char *name)
{
	unsigned long flags;
	char *value;

	spin_lock_irqsave(&nvram_lock, flags);
	value = _nvram_get(name);
	spin_unlock_irqrestore(&nvram_lock, flags);

	return value;
}

char *
nvram_get(const char *name)
{
	if (nvram_major >= 0)
		return real_nvram_get(name);
	else
		return early_nvram_get(name);
}

int
nvram_unset(const char *name)
{
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&nvram_lock, flags);
#ifdef CFE_UPDATE //unset variable in embedded nvram
        if(strncmp(name, CFE_NVRAM_PREFIX, strlen(CFE_NVRAM_PREFIX))==0)
        {
                if((ret = cfe_update(name+strlen(CFE_NVRAM_PREFIX), NULL)) == 0)
                {
                        ret = _nvram_unset(name+strlen(CFE_NVRAM_PREFIX));
                }
        }
        else
#endif
	ret = _nvram_unset(name);
	spin_unlock_irqrestore(&nvram_lock, flags);

	return ret;
}

static void
erase_callback(struct erase_info *done)
{
	wait_queue_head_t *wait_q = (wait_queue_head_t *) done->priv;
	wake_up(wait_q);
}

int
nvram_commit(void)
{
	char *buf;
	size_t erasesize, len, magic_len;
	unsigned int i;
	int ret;
	struct nvram_header *header;
	unsigned long flags;
	u_int32_t offset;
	DECLARE_WAITQUEUE(wait, current);
	wait_queue_head_t wait_q;
	struct erase_info erase;
	u_int32_t magic_offset = 0; /* Offset for writing MAGIC # */

	if (!nvram_mtd) {
		printk("nvram_commit: NVRAM not found\n");
		return -ENODEV;
	}

	if (in_interrupt()) {
		printk("nvram_commit: not committing in interrupt\n");
		return -EINVAL;
	}

	/* Backup sector blocks to be erased */
	erasesize = ROUNDUP(NVRAM_SPACE, nvram_mtd->erasesize);
	if (!(buf = kmalloc(erasesize, GFP_KERNEL))) {
		printk("nvram_commit: out of memory\n");
		return -ENOMEM;
	}

	down(&nvram_sem);

	if ((i = erasesize - NVRAM_SPACE) > 0) {
		offset = nvram_mtd->size - erasesize;
		len = 0;
		ret = nvram_mtd->read(nvram_mtd, offset, i, &len, buf);
		if (ret || len != i) {
			printk("nvram_commit: read error ret = %d, len = %d/%d\n", ret, len, i);
			ret = -EIO;
			goto done;
		}
		header = (struct nvram_header *)(buf + i);
		magic_offset = i + ((void *)&header->magic - (void *)header);
	} else {
		offset = nvram_mtd->size - NVRAM_SPACE;
		magic_offset = ((void *)&header->magic - (void *)header);
		header = (struct nvram_header *)buf;
	}

	/* clear the existing magic # to mark the NVRAM as unusable 
	 * we can pull MAGIC bits low without erase
	 */
	header->magic = NVRAM_CLEAR_MAGIC; /* All zeros magic */
	/* Unlock sector blocks */
	if (nvram_mtd->unlock)
		nvram_mtd->unlock(nvram_mtd, offset, nvram_mtd->erasesize);
	ret = nvram_mtd->write(nvram_mtd, offset + magic_offset, sizeof(header->magic),
		&magic_len, (char *)&header->magic);
	if (ret || magic_len != sizeof(header->magic)) {
		printk("nvram_commit: clear MAGIC error\n");
		ret = -EIO;
		goto done;
	}

	header->magic = NVRAM_MAGIC;
	/* reset MAGIC before we regenerate the NVRAM,
	 * otherwise we'll have an incorrect CRC
	 */
	/* Regenerate NVRAM */
	spin_lock_irqsave(&nvram_lock, flags);
	ret = _nvram_commit(header);
	spin_unlock_irqrestore(&nvram_lock, flags);
	if (ret)
		goto done;

	/* Erase sector blocks */
	init_waitqueue_head(&wait_q);
	for (; offset < nvram_mtd->size - NVRAM_SPACE + header->len;
		offset += nvram_mtd->erasesize) {

		erase.mtd = nvram_mtd;
		erase.addr = offset;
		erase.len = nvram_mtd->erasesize;
		erase.callback = erase_callback;
		erase.priv = (u_long) &wait_q;

		set_current_state(TASK_INTERRUPTIBLE);
		add_wait_queue(&wait_q, &wait);

		/* Unlock sector blocks */
		if (nvram_mtd->unlock)
			nvram_mtd->unlock(nvram_mtd, offset, nvram_mtd->erasesize);

		if ((ret = nvram_mtd->erase(nvram_mtd, &erase))) {
			set_current_state(TASK_RUNNING);
			remove_wait_queue(&wait_q, &wait);
			printk("nvram_commit: erase error\n");
			goto done;
		}

		/* Wait for erase to finish */
		schedule();
		remove_wait_queue(&wait_q, &wait);
	}

	/* Write partition up to end of data area */
	header->magic = NVRAM_INVALID_MAGIC; /* All ones magic */
	offset = nvram_mtd->size - erasesize;
	i = erasesize - NVRAM_SPACE + header->len;
	ret = nvram_mtd->write(nvram_mtd, offset, i, &len, buf);
	if (ret || len != i) {
		printk("nvram_commit: write error\n");
		ret = -EIO;
		goto done;
	}

	/* Now mark the NVRAM in flash as "valid" by setting the correct
	 * MAGIC #
	 */
	header->magic = NVRAM_MAGIC;
	ret = nvram_mtd->write(nvram_mtd, offset + magic_offset, sizeof(header->magic),
		&magic_len, (char *)&header->magic);
	if (ret || magic_len != sizeof(header->magic)) {
		printk("nvram_commit: write MAGIC error\n");
		ret = -EIO;
		goto done;
	}

	offset = nvram_mtd->size - erasesize;
	ret = nvram_mtd->read(nvram_mtd, offset, 4, &len, buf);

done:
	up(&nvram_sem);
	kfree(buf);
	return ret;
}

int
nvram_getall(char *buf, int count)
{
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&nvram_lock, flags);
	if (nvram_major >= 0)
		ret = _nvram_getall(buf, count);
	else
		ret = early_nvram_getall(buf, count);
	spin_unlock_irqrestore(&nvram_lock, flags);

	return ret;
}

EXPORT_SYMBOL(nvram_init);
EXPORT_SYMBOL(nvram_get);
EXPORT_SYMBOL(nvram_getall);
EXPORT_SYMBOL(nvram_set);
EXPORT_SYMBOL(nvram_unset);
EXPORT_SYMBOL(nvram_commit);

/* User mode interface below */

static ssize_t
dev_nvram_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
	char tmp[100], *name = tmp, *value;
	ssize_t ret;
	unsigned long off;

	if (count > sizeof(tmp)) {
		if (!(name = kmalloc(count, GFP_KERNEL)))
			return -ENOMEM;
	}

	if (copy_from_user(name, buf, count)) {
		ret = -EFAULT;
		goto done;
	}

	if (*name == '\0') {
		/* Get all variables */
		ret = nvram_getall(name, count);
		if (ret == 0) {
			if (copy_to_user(buf, name, count)) {
				ret = -EFAULT;
				goto done;
			}
			ret = count;
		}
	} else {
		if (!(value = nvram_get(name))) {
			ret = 0;
			goto done;
		}

		/* Provide the offset into mmap() space */
		off = (unsigned long) value - (unsigned long) nvram_buf;

		if (put_user(off, (unsigned long *) buf)) {
			ret = -EFAULT;
			goto done;
		}

		ret = sizeof(unsigned long);
	}

	flush_cache_all();

done:
	if (name != tmp)
		kfree(name);

	return ret;
}

static ssize_t
dev_nvram_write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{
	char tmp[100], *name = tmp, *value;
	ssize_t ret;

	if (count > sizeof(tmp)) {
		if (!(name = kmalloc(count, GFP_KERNEL)))
			return -ENOMEM;
	}

	if (copy_from_user(name, buf, count)) {
		ret = -EFAULT;
		goto done;
	}

	value = name;
	name = strsep(&value, "=");
	if (value)
		ret = nvram_set(name, value) ? : count;
	else
		ret = nvram_unset(name) ? : count;

done:
	if (name != tmp)
		kfree(name);

	return ret;
}

static int
dev_nvram_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	if (cmd != NVRAM_MAGIC)
		return -EINVAL;

#ifndef NLS_XFR
	return nvram_commit();
#else
	if(arg == 0)
		return nvram_commit();
	else
		return nvram_xfr((char *)arg);
#endif	// NLS_XFR
}

static int
dev_nvram_mmap(struct file *file, struct vm_area_struct *vma)
{
	unsigned long offset = __pa(nvram_buf) >> PAGE_SHIFT;

	if (remap_pfn_range(vma, vma->vm_start, offset,
	                    vma->vm_end - vma->vm_start,
	                    vma->vm_page_prot))
		return -EAGAIN;

	return 0;
}

static int
dev_nvram_open(struct inode *inode, struct file * file)
{
	return 0;
}

static int
dev_nvram_release(struct inode *inode, struct file * file)
{
	return 0;
}

static struct file_operations dev_nvram_fops = {
	owner:		THIS_MODULE,
	open:		dev_nvram_open,
	release:	dev_nvram_release,
	read:		dev_nvram_read,
	write:		dev_nvram_write,
	ioctl:		dev_nvram_ioctl,
	mmap:		dev_nvram_mmap
};

static void
dev_nvram_exit(void)
{
	int order = 0;
	struct page *page, *end;

	if (nvram_class) {
		class_device_destroy(nvram_class, MKDEV(nvram_major, 0));
		class_destroy(nvram_class);
	}

	if (nvram_major >= 0)
		unregister_chrdev(nvram_major, "nvram");

	if (nvram_mtd)
		put_mtd_device(nvram_mtd);

	while ((PAGE_SIZE << order) < NVRAM_SPACE)
		order++;
	end = virt_to_page(nvram_buf + (PAGE_SIZE << order) - 1);
	for (page = virt_to_page(nvram_buf); page <= end; page++)
		ClearPageReserved(page);

	_nvram_exit();
}

static int
dev_nvram_init(void)
{
	int order = 0, ret = 0;
	struct page *page, *end;
	unsigned int i;
	osl_t *osh;

	/* Allocate and reserve memory to mmap() */
	while ((PAGE_SIZE << order) < NVRAM_SPACE)
		order++;
	end = virt_to_page(nvram_buf + (PAGE_SIZE << order) - 1);
	for (page = virt_to_page(nvram_buf); page <= end; page++) {
		SetPageReserved(page);
	}

#if defined(CONFIG_MTD) || defined(CONFIG_MTD_MODULE)
	/* Find associated MTD device */
	for (i = 0; i < MAX_MTD_DEVICES; i++) {
		nvram_mtd = get_mtd_device(NULL, i);
		if (!IS_ERR(nvram_mtd)) {
			if (!strcmp(nvram_mtd->name, "nvram") &&
			    nvram_mtd->size >= NVRAM_SPACE) {
				break;
			}
			put_mtd_device(nvram_mtd);
		}
	}
	if (i >= MAX_MTD_DEVICES)
		nvram_mtd = NULL;
#endif

	/* Initialize hash table lock */
	spin_lock_init(&nvram_lock);

	/* Initialize commit semaphore */
	init_MUTEX(&nvram_sem);

	/* Register char device */
	if ((nvram_major = register_chrdev(0, "nvram", &dev_nvram_fops)) < 0) {
		ret = nvram_major;
		goto err;
	}

	if (si_osh(sih) == NULL) {
		osh = osl_attach(NULL, SI_BUS, FALSE);
		if (osh == NULL) {
			printk("Error allocating osh\n");
			unregister_chrdev(nvram_major, "nvram");
			goto err;
		}
		si_setosh(sih, osh);
	}

	/* Initialize hash table */
	_nvram_init(sih);

	/* Create /dev/nvram handle */
	nvram_class = class_create(THIS_MODULE, "nvram");
	if (IS_ERR(nvram_class)) {
		printk("Error creating nvram class\n");
		goto err;
	}

	/* Add the device nvram0 */
	class_device_create(nvram_class, NULL, MKDEV(nvram_major, 0), NULL, "nvram");

	/* Set the SDRAM NCDL value into NVRAM if not already done */
	if (getintvar(NULL, "sdram_ncdl") == 0) {
		unsigned int ncdl;
		char buf[] = "0x00000000";

		if ((ncdl = si_memc_get_ncdl(sih))) {
			sprintf(buf, "0x%08x", ncdl);
			nvram_set("sdram_ncdl", buf);
			nvram_commit();
		}
	}

	return 0;

err:
	dev_nvram_exit();
	return ret;
}

#ifdef CFE_UPDATE
int get_embedded_block(struct mtd_info *mtd, char *buf, size_t erasesize,
                       u_int32_t *offset, struct nvram_header **header, u_int32_t *emb_size)
{
        size_t len;
        struct nvram_header *nvh;

#ifdef CONFIG_RTAN23 /*for AMCC RTAN23 */
        *offset = mtd->size - erasesize; /*/at the end of mtd */
        *emb_size = 8*1024 - 16; /*/8K - 16 byte */
        printk("get_embedded_block: mtd->size(%08x) erasesize(%08x) offset(%08x) emb_size(%08x)\n", mtd->size, erasesize, *offset, *emb_size);
        cfe_mtd->read(mtd, *offset, erasesize, &len, buf);
        if(len != erasesize)
                return -EIO;

        /* find nvram header */
        nvh = (struct nvram_header *)(buf + erasesize - 8*1024);
        if (nvh->magic == NVRAM_MAGIC)
        {
                *header = nvh;
                return 0;
        }

#else /* for Broadcom WL500 serials */
        *offset = 0; /* from the mtd start */
        *emb_size = 4096; /* 1K byte */
        printk("get_embedded_block: mtd->size(%08x) erasesize(%08x) offset(%08x) emb_size(%08x)\n", mtd->size, erasesize, *offset, *emb_size);
        cfe_mtd->read(mtd, *offset, erasesize, &len, buf);
        if(len != erasesize)
                return -EIO;

        /* find nvram header */
        nvh = (struct nvram_header *)(buf + (4 * 1024));
        if (nvh->magic == NVRAM_MAGIC)
        {
                *header = nvh;
                return 0;
        }
        nvh = (struct nvram_header *)(buf + (1 * 1024));
        if (nvh->magic == NVRAM_MAGIC)
        {
                *header = nvh;
                return 0;
        }
#endif
        printk("get_embedded_block: no nvram magic found\n");
        return -ENXIO;
}
static int cfe_init(void)
{
        size_t erasesize;
        int i;
        int ret = 0;

        /* Find associated MTD device */
        for (i = 0; i < MAX_MTD_DEVICES; i++) {
                cfe_mtd = get_mtd_device(NULL, i);
                if (cfe_mtd != NULL) {
                        printk("cfe_init: CFE MTD %x %s %x\n", i, cfe_mtd->name, cfe_mtd->size);
                        if (!strcmp(cfe_mtd->name, "boot"))
                                break;
                        put_mtd_device(cfe_mtd);
                }
        }
        if (i >= MAX_MTD_DEVICES)
        {
                printk("cfe_init: No CFE MTD\n");
                cfe_mtd = NULL;
                ret = -ENODEV;
        }

        if(cfe_mtd == NULL) goto fail;

        /* sector blocks to be erased and backup */
        erasesize = ROUNDUP(CFE_NVRAM_SPACE, cfe_mtd->erasesize);

	//printk("cfe_init: block size %d\n", erasesize);
        cfe_buf = kmalloc(erasesize, GFP_KERNEL);

        if(cfe_buf == NULL)
        {
                //printk("cfe_init: No CFE Memory\n");
                ret = -ENOMEM;
                goto fail;
        }
        if((ret = get_embedded_block(cfe_mtd, cfe_buf, erasesize, &cfe_offset, &cfe_nvram_header, &cfe_embedded_size)))
                goto fail;

        printk("cfe_init: cfe_nvram_header(%08x)\n", (unsigned int) cfe_nvram_header);
	bcm947xx_watchdog_disable();

        return 0;

fail:
        if (cfe_mtd != NULL)
        {
                put_mtd_device(cfe_mtd);
                cfe_mtd=NULL;
        }
        if(cfe_buf != NULL)
        {
                kfree(cfe_buf);
                cfe_buf=NULL;
        }
        return ret;
}
static int cfe_update(const char *keyword, const char *value)
{
        struct nvram_header *header;
        uint8 crc;
        int ret;
        int found = 0;
        char *str, *end, *mv_target = NULL, *mv_start = NULL;

        if(keyword == NULL || *keyword == 0)
                return 0;

        if(cfe_buf == NULL||cfe_mtd == NULL)
                if((ret = cfe_init()))
                        return ret;

        header = cfe_nvram_header;

	//printk("cfe_update: before %x %x\n", header->len,  cfe_nvram_header->crc_ver_init&0xff);
        str = (char *) &header[1];
        end = (char *) header + cfe_embedded_size - 2;
        end[0] = end[1] = '\0';
        for (; *str; str += strlen(str) + 1)
        {
                if(!found)
                {
                        if(strncmp(str, keyword, strlen(keyword)) == 0 && str[strlen(keyword)] == '=')
                        {
                                printk("cfe_update: !!!! found !!!!\n");
                                found = 1;
                                if(value != NULL && strlen(str) == strlen(keyword) + 1 + strlen(value))
                                {//string length is the same
                                        strcpy(str+strlen(keyword)+1, value);
                                }
                                else
                                {
                                        mv_target = str;
                                        mv_start = str + strlen(str) + 1;
                                }
                        }
                }
        }
        /* str point to the end of all embedded nvram settings */

        if(mv_target != NULL)
        { /* need to move string */
                int str_len = strlen(mv_target);
                printk("cfe_update: mv_target(%08x) mv_start(%08x) str(%08x) str_len(%d)\n", (unsigned int)mv_target, (unsigned int)mv_start, (unsigned int)str, str_len);
                if(value != NULL && (str + strlen(keyword) + 1 + strlen(value) + 1 - (str_len + 1)) > end)
                        return -ENOSPC;
                memmove(mv_target, mv_start, str - mv_start);
                printk("cfe_update: memmove done\n");
                str -= (str_len + 1); /* /set str to the end for placing incoming keyword and value there */
        }

        if(value == NULL)
        {
                printk("cfe_update: do unset\n");
        }
        else if(!found || mv_target != NULL) /*new or movement */
        { /* append the keyword and value here */
                printk("cfe_update: str(%08x)\n", (unsigned int) str);
                if((str + strlen(keyword) + 1 + strlen(value) + 1) > end)
                        return -ENOSPC;
                str += sprintf(str, "%s=%s", keyword, value) + 1;
                printk("cfe_update: append string\n");
        }
/* calc length */
        memset(str, 0, cfe_embedded_size+(char *)header - str);
        str += 2;
        header->len = ROUNDUP(str - (char *) header, 4);
        printk("cfe_update: header len: %x\n", header->len);
/*/calc crc */
        crc = nvram_calc_crc(header);
        printk("cfe_update: nvram_calc_crc(header) = 0x%02x\n", crc);
        header->crc_ver_init = (header->crc_ver_init & NVRAM_CRC_VER_MASK)|crc;
        /*/printk("cfe_update: after %x %x\n", header->crc_ver_init&0xFF, crc); */
        return 0;
}
static int cfe_dump(void)
{
        unsigned int i;
        int ret;
        unsigned char *ptr;

        if(cfe_buf == NULL||cfe_mtd == NULL)
                if((ret = cfe_init()))
                        return ret;

        printk("cfe_dump: cfe_buf(%08x), dump 1024 byte\n", (unsigned int)cfe_buf);
        for(i=0, ptr=(unsigned char *)cfe_nvram_header - 1024; ptr < (unsigned char *)cfe_nvram_header; i++, ptr++)
        {
                if(i%16==0) printk("%04x: %02x ", i, *ptr);
                else if(i%16==15) printk("%02x\n", *ptr);
                else if(i%16==7) printk("%02x - ", *ptr);
                else printk("%02x ", *ptr);
        }

        printk("\ncfe_dump: cfe_nvram_header(%08x)\n", (unsigned int)cfe_nvram_header);
        printk("cfe_dump: cfe_nvram_header->len(0x%08x)\n", cfe_nvram_header->len);

        printk("\n####################\n");
        for(i=0, ptr=(unsigned char *)cfe_nvram_header; i< cfe_embedded_size; i++, ptr++)
        {
                if(i%16==0) printk("%04x: %02x ", i, *ptr);
                else if(i%16==15) printk("%02x\n", *ptr);
                else if(i%16==7) printk("%02x - ", *ptr);
                else printk("%02x ", *ptr);
        }
        printk("\n####################\n");
        ptr = (unsigned char *)&cfe_nvram_header[1];
        while(*ptr)
        {
                printk("%s\n", ptr);
                ptr += strlen(ptr) + 1;
        }
        printk("\n####################\n");
        for(i=0, ptr=((unsigned char *)cfe_nvram_header) + cfe_embedded_size; i<16; i++, ptr++)
        {
                if(i%16==0) printk("%04x: %02x ", i, *ptr);
                else if(i%16==15) printk("%02x\n", *ptr);
                else if(i%16==7) printk("%02x - ", *ptr);
                else printk("%02x ", *ptr);
        }
        return 0;
}

static int cfe_commit(void)
{
        DECLARE_WAITQUEUE(wait, current);
        wait_queue_head_t wait_q;
        struct erase_info erase;
        int ret = 0;
        size_t erasesize, len=0;
        u_int32_t offset;

        if(cfe_mtd == NULL||cfe_buf == NULL)
        {
                printk("cfe_commit: do nothing\n");
                return 0;
        }

#if 0
        ret = cfe_dump();
        return ret;
#endif
#if 1
        /* Backup sector blocks to be erased */
        erasesize = ROUNDUP(CFE_NVRAM_SPACE, cfe_mtd->erasesize);
        printk("cfe_commit: erasesize(%08x) cfe_offset(%08x)\n", erasesize, cfe_offset);

        /* Erase sector blocks */
        init_waitqueue_head(&wait_q);
        for (offset=cfe_offset;offset < cfe_offset+erasesize;offset += cfe_mtd->erasesize) {
           printk("cfe_commit: ERASE sector block offset(%08x) cfe_mtd->erasesize(%08x)\n", offset, cfe_mtd->erasesize);
           erase.mtd = cfe_mtd;
           erase.addr = offset;
           erase.len = cfe_mtd->erasesize;
           erase.callback = erase_callback;
           erase.priv = (u_long) &wait_q;

           set_current_state(TASK_INTERRUPTIBLE);
           add_wait_queue(&wait_q, &wait);
           /* Unlock sector blocks */
           if (cfe_mtd->unlock)
                   cfe_mtd->unlock(cfe_mtd, offset, cfe_mtd->erasesize);

           if ((ret = cfe_mtd->erase(cfe_mtd, &erase))) {
                set_current_state(TASK_RUNNING);
                remove_wait_queue(&wait_q, &wait);
                printk("cfe_commit: erase error\n");
                ret = -EIO;
                goto done;
           }

           /* Wait for erase to finish */
           schedule();
           remove_wait_queue(&wait_q, &wait);
        }

        ret = cfe_mtd->write(cfe_mtd, cfe_offset, erasesize, &len, cfe_buf);
        printk("cfe_commit: MTD_WRITE cfe_offset(%08x) erasesize(%08x) len(%08x) ret(%08x)\n", cfe_offset, erasesize, len, ret);

        if (ret || len != erasesize) {
           printk("cfe_commit: write error\n");
           ret = -EIO;
        }

done:
        if (cfe_mtd != NULL)
        {
                put_mtd_device(cfe_mtd);
                cfe_mtd=NULL;
        }
        if(cfe_buf != NULL)
        {
                kfree(cfe_buf);
                cfe_buf=NULL;
        }
        //printk("commit: %d\n", ret);
        return ret;
#endif
}
#endif


//#ifdef CFE_UPDATE
#if 0
void cfe_init(void)
{
        size_t erasesize, len;
        int i;

        /* Find associated MTD device */
        for (i = 0; i < MAX_MTD_DEVICES; i++) {
                cfe_mtd = get_mtd_device(NULL, i);
                if (cfe_mtd) {
                        printk("CFE MTD: %x %s %x\n", i, cfe_mtd->name, cfe_mtd->size);
                        if (!strcmp(cfe_mtd->name, "boot"))
                                break;
                        put_mtd_device(cfe_mtd);
                }
        }
        if (i >= MAX_MTD_DEVICES)
        {
                printk("No CFE MTD\n");
                cfe_mtd = NULL;
        }

        if(!cfe_mtd) goto fail;

        /* sector blocks to be erased and backup */
        erasesize = ROUNDUP(CFE_NVRAM_SPACE, cfe_mtd->erasesize);

        /* printk("block size %d\n", erasesize); */

        cfe_buf = kmalloc(erasesize, GFP_KERNEL);

        if(!cfe_buf)
        {
                /* printk("No CFE Memory\n"); */
                goto fail;
        }
        cfe_mtd->read(cfe_mtd, CFE_NVRAM_START, erasesize, &len, cfe_buf);

        /* find nvram header */
        for(i=0;i<len;i+=4)
        {
                cfe_nvram_header=(struct nvram_header *)&cfe_buf[i];
                if (cfe_nvram_header->magic==NVRAM_MAGIC) break;
        }

        bcm947xx_watchdog_disable(); /*disable watchdog as well */

#if 0                                                           
        printf("read from nvram %d %s\n", i, cfe_buf);
        for(i=0;i<CFE_SPACE;i++)
        {
              if(i%16) printk("\n");
              printk("%02x ", (unsigned char)cfe_buf[i]);
        }
#endif
        return;
fail:
        if (cfe_mtd)
        {
                put_mtd_device(cfe_mtd);
                cfe_mtd=NULL;
        }
        if(cfe_buf)
        {
                kfree(cfe_buf);
                cfe_buf=NULL;
        }
        return;
}

void cfe_update(const char *keyword, const char *value)
{
        unsigned long i, offset;
        struct nvram_header tmp, *header;
        uint8 crc;
//        int ret;
        int found = 0;

        if(!cfe_buf||!cfe_mtd)
                cfe_init();

        if (!cfe_buf||!cfe_mtd) return;

        header = cfe_nvram_header;

        printk("before: %x %x\n", header->len,  cfe_nvram_header->crc_ver_init&0xff); 

        for(i=CFE_NVRAM_START;i<=CFE_NVRAM_END;i++)
        {
                if(strncmp(&cfe_buf[i], keyword, strlen(keyword))==0)
                {
                        printk("before: %s\n", cfe_buf+i); 
                        offset=strlen(keyword);
                        memcpy(cfe_buf+i+offset+1, value, strlen(value));
                        printk("after: %s\n", cfe_buf+i); 
                        found = 1;
                }
        }

        if(!found)
        {
                char *tmp_buf = (char *)cfe_nvram_header;

                /* printk("header len: %x\n", header->len); */
                sprintf(tmp_buf+header->len, "%s=%s", keyword, value);
                header->len = header->len + strlen(keyword) + strlen(value) + 2;
                /* printk("header len: %x\n", header->len); */
        }

        tmp.crc_ver_init = htol32(header->crc_ver_init);
        tmp.config_refresh = htol32(header->config_refresh);
        tmp.config_ncdl = htol32(header->config_ncdl);
        crc = hndcrc8((char *) &tmp + 9, sizeof(struct nvram_header) - 9, CRC8_INIT_VALUE);

        /* Continue CRC8 over data bytes */
        crc = hndcrc8((char *) &header[1], header->len - sizeof(struct nvram_header), crc);
        header->crc_ver_init = (header->crc_ver_init&0xFFFFFF00)|crc;
        printk("after: %x %x\n", header->crc_ver_init&0xFF, crc); 
}

int cfe_commit(void)
{
        DECLARE_WAITQUEUE(wait, current);
        wait_queue_head_t wait_q;
        struct erase_info erase;
//        unsigned int i;
        int ret;
        size_t erasesize, len;
        u_int32_t offset;
//        char *buf;

        if(!cfe_buf||!cfe_mtd) cfe_init();

        if(!cfe_mtd||!cfe_buf)
        {
                ret = - ENOMEM;
                goto done;
        }

        /* Backup sector blocks to be erased */
        erasesize = ROUNDUP(CFE_NVRAM_SPACE, cfe_mtd->erasesize);

	down(&nvram_sem);

        /* Erase sector blocks */
        init_waitqueue_head(&wait_q);


        for (offset=CFE_NVRAM_START;offset <= CFE_NVRAM_END;offset += cfe_mtd->erasesize) {
           erase.mtd = cfe_mtd;
           erase.addr = offset;
           erase.len = cfe_mtd->erasesize;
           erase.callback = erase_callback;
           erase.priv = (u_long) &wait_q;

           set_current_state(TASK_INTERRUPTIBLE);
           add_wait_queue(&wait_q, &wait);
           /* Unlock sector blocks */
           if (cfe_mtd->unlock)
                   cfe_mtd->unlock(cfe_mtd, offset, cfe_mtd->erasesize);

           if ((ret = cfe_mtd->erase(cfe_mtd, &erase))) {
                set_current_state(TASK_RUNNING);
                remove_wait_queue(&wait_q, &wait);
                printk("cfe_commit: erase error\n");
                goto done;
           }

           /* Wait for erase to finish */
           schedule();
           remove_wait_queue(&wait_q, &wait);
        }

        ret = cfe_mtd->write(cfe_mtd, CFE_NVRAM_START, erasesize, &len, cfe_buf);
        /* printk("Write offset: %x %x %x\n", ret, len, erasesize); */

        if (ret || len != erasesize) {
           printk("cfe_commit: write error\n");
           ret = -EIO;
        }

done:
	up(&nvram_sem);
        if (cfe_mtd)
        {
                put_mtd_device(cfe_mtd);
                cfe_mtd=NULL;
        }
        if(cfe_buf)
        {
                kfree(cfe_buf);
                cfe_buf=NULL;
        }
        /* printk("commit: %d\n", ret); */
        return ret;

}
#endif


module_init(dev_nvram_init);
module_exit(dev_nvram_exit);
