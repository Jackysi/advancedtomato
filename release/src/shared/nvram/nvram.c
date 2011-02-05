/*
 * NVRAM variable manipulation (common)
 *
 * Copyright 2004, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id$
 */

#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <linux/delay.h>
#include <bcmendian.h>
#include <bcmnvram.h>
#include <sbsdram.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
#define get_seconds() CURRENT_TIME
#define jiffy_time (jiffies)
#else
#define jiffy_time (jiffies - INITIAL_JIFFIES)
#endif


extern int _nvram_init_read(void);
extern int nvram_space;		/* Size of the NVRAM.  Generally 32kb */
extern char nvram_buf[];	/* The buffer to hold values. */
extern char *nvram_commit_buf;	/* Buffer for the flash eraseblock(s). */
extern int oflow_area_present;

char *_nvram_get(const char *name);
int _nvram_set(const char *name, const char *value);
int _nvram_unset(const char *name);
int _nvram_getall(char *buf, int count);
int _nvram_commit(struct nvram_header *header);
int _nvram_init(void *sb);
void _nvram_exit(void);
uint8 nvram_calc_crc(struct nvram_header * nvh);
int walk_chain(int pr);
struct nvram_dbitem *_nvram_realloc(struct nvram_dbitem *t, const char *name,
                                           const char *value);
void _nvram_free(struct nvram_dbitem *t);
void _nvram_valbuf_compactify(void);

#define NUM_HLH 257
static struct nvram_dbitem *BCMINITDATA(nvram_hash)[NUM_HLH];
static struct nvram_dbitem *nvram_dead;
static int it_siz, it_cnt;
static unsigned nvram_offset = 0;
char sbuf[128];

/* Prio == write priority (actually: category)
 *   * Entries that must be in the main (original) NVRAM area, visible
 *		to PMON & CFE.
 *   * Entries that can go anywhere.  PMON & CFE don't need to see these.
 *
 * Since there is no spec for what PMON & CFE need to see, and the list
 * may change for different firmware versions:
 * Assume that when "restore_defaults" != "0" then CFE cleared
 * the NVRAM and set its defaults.
 * Put a special flag item that is physically after the prio 1 items.
 * When refreshing frm the flash, everything before this is defined as prio 1.
 * Everything after it is prio 2.
 * If it is not found, then everything is prio 1.
 * When writing out (commit), write the prio 1 items, then the flag item,
 * then everything else.
 */

static uint16 prio;
#define PRIO_MARK_ITEM "}Marker42"
#define PRIO_MAIN 1
#define PRIO_ANYWHERE 2
#define PRIO_OFLOW 3

int prefer_ov = 0;	/* Prefer "any" go to oflow area? */
int oflow_ok = 1;	/* It is okay to go into oflow area? */
int alt = 0;		// temp

/* Free all tuples. Should be locked. */
static void  
BCMINITFN(nvram_free)(void)
{
	uint i;
	struct nvram_dbitem *t, *next;

	/* Free hash table */
	for (i = 0; i < NUM_HLH; i++) {
		for (t = nvram_hash[i]; t; t = next) {
			next = t->next;
			_nvram_free(t);
		}
		nvram_hash[i] = NULL;
	}

	/* Free dead table */
	for (t = nvram_dead; t; t = next) {
		next = t->next;
		_nvram_free(t);
	}
	nvram_dead = NULL;

	/* Indicate to per-port code that all tuples have been freed */
	_nvram_free(NULL);
}

/* String hash. FNV-1a algorithm */
static INLINE uint
hash(const char *s)
{
	unsigned hval = 0x811c9dc5;

	while (*s) {
		hval ^= *s++;
		hval += (hval<<1) + (hval<<4) + (hval<<7) + (hval<<8) + (hval<<24);
	}
	return hval;
}

/* (Re)initialize the hash table. Should be locked. */
static int 
BCMINITFN(nvram_rehash)(struct nvram_header *header)
{
	char buf[20], *name, *value, *end, *eq;
	int n = 0;	//temp

	/* (Re)initialize hash table */
	nvram_free();

	/* Parse and set "name=value\0 ... \0\0" */
	name = (char *) &header[1];
	end = (char *) header + nvram_space - 2;
	end[0] = end[1] = '\0';
	for (; *name; name = value + strlen(value) + 1) {
		if (!(eq = strchr(name, '=')))
			break;
		++n;
		*eq = '\0';
		value = eq + 1;
		_nvram_set(name, value);
		*eq = '=';
	}

	/* Set special SDRAM parameters */
	if (!_nvram_get("sdram_init")) {
		sprintf(buf, "0x%04X", (uint16)(header->crc_ver_init >> 16));
		_nvram_set("sdram_init", buf);
	}
	if (!_nvram_get("sdram_config")) {
		sprintf(buf, "0x%04X", (uint16)(header->config_refresh & 0xffff));
		_nvram_set("sdram_config", buf);
	}
	if (!_nvram_get("sdram_refresh")) {
		sprintf(buf, "0x%04X", (uint16)((header->config_refresh >> 16) & 0xffff));
		_nvram_set("sdram_refresh", buf);
	}
	if (!_nvram_get("sdram_ncdl")) {
		sprintf(buf, "0x%08X", header->config_ncdl);
		_nvram_set("sdram_ncdl", buf);
	}
	printk("Item count: %d  valsiz: %u\n", n, nvram_offset);
	return 0;
}

/* Get the value of an NVRAM variable. Should be locked. */
char * 
_nvram_get(const char *name)
{
	uint i;
	struct nvram_dbitem *t, **prev;

	if (!name)
		return NULL;

	if (name[0] == '=') {	/* Special dynamic size info */
		if (strcmp(name, "=nvram_used") == 0) {
			sprintf(&nvram_buf[nvram_space - 128], "%d",
				walk_chain(0));
			return (&nvram_buf[nvram_space - 128]);
		}
		if (strcmp(name, "=nvram_space") == 0) {
			i = nvram_space -1;	/* Trailing null but no pad. */
			if (oflow_area_present)
				i += NVRAM_32K -1;
			sprintf(&nvram_buf[nvram_space - 128], "%d", i);
			return (&nvram_buf[nvram_space - 128]);
		}
	}

	/* Hash the name */
	i = hash(name) % NUM_HLH;

	/* Find the item in the hash table */
	for (prev = &nvram_hash[i], t = *prev;
		t && (strcmp(t->name, name));
		prev = &t->next, t = *prev) {}
	if (t) {
		*prev = t->next;	/* Move it to the top of the chain - MRU. */
		t->next = nvram_hash[i];
		nvram_hash[i] = t;
		return(t->value);
	}
	return NULL;
}

/* Set the value of an NVRAM variable. Should be locked. */
int 
BCMINITFN(_nvram_set)(const char *name, const char *value)
{
	uint i;
	struct nvram_dbitem *t, *u, **prev;

	/* Special control items.  No name.  Value is control command.
	 * prio_main, _any , oflow   Says to flag subsequent sets accordingly.
	 * "=name" says to set prio on that name. */
	if (*name == '\0') {
		printk("Special: '%s'\n", value);
		if (strcmp(value, "prio_main") == 0)
			prio = PRIO_MAIN;
		else if (strcmp(value, "prio_any") == 0)
			prio = PRIO_ANYWHERE;
		else if (strcmp(value, "prio_oflow") == 0)
			prio = PRIO_OFLOW;
		else if (*value == '=') {	/* Set prio on this. */
			name = value +1;
			i = hash(name) % NUM_HLH;
			for (prev = &nvram_hash[i], t = *prev;
			     t && (strcmp(t->name, name));
			     prev = &t->next, t = *prev) {}
			if (t)
				t->prio = prio;
		}
		else if (strcmp(value, "prefer_oflow-n") == 0)
			prefer_ov = 0;
		else if (strcmp(value, "prefer_oflow-y") == 0)
			prefer_ov = 1;
		else if (strcmp(value, "prefer_oflow-n") == 0) alt = 0; //temp
		else if (strcmp(value, "prefer_oflow-a") == 0) alt = 1; //temp
		else if (strcmp(value, "oflow_ok-n") == 0)
			oflow_ok = 0;
		else if (strcmp(value, "oflow_ok-y") == 0)
			oflow_ok = 1;
		else if (strcmp(value, "reset_stat") == 0) {
			_nvram_unset("z-commit");
			for (i = 0; ++i < 50; ) {
				sprintf(sbuf, "z-commit_%02u", i);
				_nvram_unset(sbuf);
			}
		}
		else
			printk("nvram: Unknown special value '%s'\n", value);
		return 0;
	}

	/* Hash the name */
	i = hash(name) % NUM_HLH;

	/* Find the item in the hash table */
	for (prev = &nvram_hash[i], t = *prev;
	     t && (strcmp(t->name, name));
	     prev = &t->next, t = *prev) {}

	/* (Re)allocate tuple */
	if (!(u = _nvram_realloc(t, name, value)))
		return -12; /* -ENOMEM */

	/* Value reallocated */
	if (t && t == u) {
		*prev = u->next;   /* Move it to the top of the chain. */
		u->next = nvram_hash[i];
		nvram_hash[i] = u;
		return 0;
	}

#if 0
	/* Move old tuple to the dead table */
	// Can never get here!!  It would mean that the node existed but
	// nvram_realloc returned a different one.  But it doesn't.
	if (t) {
		*prev = t->next;
		t->next = nvram_dead;
		nvram_dead = t;
	}
#endif
	/* Add new tuple to the hash table */
	u->next = nvram_hash[i];
	nvram_hash[i] = u;

	return 0;
}

/* Unset the value of an NVRAM variable. Should be locked. */
int 
BCMINITFN(_nvram_unset)(const char *name)
{
	uint i;
	struct nvram_dbitem *t, **prev;

	if (!name)
		return 0;

	/* Hash the name */
	i = hash(name) % NUM_HLH;

	/* Find the item in the hash table */
	for (prev = &nvram_hash[i], t = *prev;
	     t && (strcmp(t->name, name));
	     prev = &t->next, t = *prev) {}

	/* Move it to the dead table */
	if (t) {
		*prev = t->next;
		t->next = nvram_dead;
		nvram_dead = t;
	}
	return 0;
}

/* Get all NVRAM variables. Should be locked. */
int 
_nvram_getall(char *buf, int count)
{
	uint i;
	struct nvram_dbitem *t;
	int len = 0;

	bzero(buf, count);

	/* Write name=value\0 ... \0\0 */
	for (i = 0; i < NUM_HLH; i++) {
		for (t = nvram_hash[i]; t; t = t->next) {
			if ((count - len) > (strlen(t->name) + 1 + strlen(t->value) + 1))
				len += sprintf(buf + len, "%s=%s", t->name, t->value) + 1;
			else
				break;
		}
	}
	_nvram_valbuf_compactify();	//temp
	return 0;
}

/* Regenerate NVRAM. Should be locked. */
int
BCMINITFN(_nvram_commit)(struct nvram_header *header)
{
	char *init, *config, *refresh, *ncdl;
	char *ptr;
	unsigned int i;
	int rem1, rem2;
	int siz;
	int cnt = 0, n = jiffies;	// temp
	struct nvram_dbitem *t;

	/* Regenerate header */
	header->magic = NVRAM_MAGIC;
	header->crc_ver_init = (NVRAM_VERSION << 8);
	if (!(init = _nvram_get("sdram_init")) ||
	    !(config = _nvram_get("sdram_config")) ||
	    !(refresh = _nvram_get("sdram_refresh")) ||
	    !(ncdl = _nvram_get("sdram_ncdl"))) {
		header->crc_ver_init |= SDRAM_INIT << 16;
		header->config_refresh = SDRAM_CONFIG;
		header->config_refresh |= SDRAM_REFRESH << 16;
		header->config_ncdl = 0;
	} else {
		header->crc_ver_init |= (bcm_strtoul(init, NULL, 0) & 0xffff) << 16;
		header->config_refresh = bcm_strtoul(config, NULL, 0) & 0xffff;
		header->config_refresh |= (bcm_strtoul(refresh, NULL, 0) & 0xffff) << 16;
		header->config_ncdl = bcm_strtoul(ncdl, NULL, 0);
	}
#if 1
	/* Keep info on the commits, for development debugging. */
	i = n;	//temp
	i = 0;

	if ((ptr = _nvram_get("z-commit")) != NULL)
		i = simple_strtol(ptr, NULL, 10);
	++i;
	sprintf(sbuf, "%02u   it_cnt, it_siz, uptime (msec), time(sec)", i);
	_nvram_set("z-commit", sbuf);
	if (i < 50) {
		walk_chain(0);
		sprintf(sbuf, "z-commit_%02u", i);
		sprintf(sbuf +20, "%4d,%6d,%8u, %lu", it_cnt, it_siz, jiffies_to_msecs(jiffy_time), get_seconds());
		_nvram_set(sbuf, sbuf +20);
	}
#endif
	/* Clear data area */
	/* Leave space for a closing NUL & roundup at the end */
	rem1 = nvram_space -1 -3 - sizeof(struct nvram_header);
	rem2 = 0;
	ptr = (char *)header + sizeof(struct nvram_header);
	memset(ptr, 0xff, nvram_space - sizeof(struct nvram_header));

	/* Write out all tuples */
	for (i = 0; i < NUM_HLH; i++) {
		for (t = nvram_hash[i]; t; t = t->next) {
			++cnt;
			siz = strlen(t->name) + strlen(t->value) + 2;
			if (siz > rem1) {
				printk("NVRAM overflow at %s=%-15.15s\n", t->name, t->value); 
				break;
			}
			ptr += sprintf(ptr, "%s=%s", t->name, t->value) + 1;
			rem1 -= siz;
		}
	}

	*ptr++ = 0;	/* Ends with an extra NUL */
	header->len = ROUNDUP(ptr - (char *)header, 4);
	header->crc_ver_init |= nvram_calc_crc(header);

	/* Reinitialize hash table.  Why?? Just to check. And purge. */
	nvram_rehash(header);
	return 0;
}

/* Initialize hash table. Should be locked. */
int 
BCMINITFN(_nvram_init)(void *sb)
{
	int ret;
	struct nvram_header *header;

	printk("jiffies: %lu  msec: %u\n", jiffy_time, jiffies_to_msecs(jiffy_time)); //temp
	ret = _nvram_init_read();
	if (ret >= 0) {
		header = (struct nvram_header *)(nvram_commit_buf + ret);
		nvram_rehash(header);
#if 0
		if (j >= 32 KB) {
			oflow_area_present = 1;
			ov_hdr = (struct nvram_header *)(nvram_commit_buf);
			nvram_rehash_ov(ov_hdr, header);
		}
#endif
	}
	return(ret);
}

/* Free hash table. Should be locked. */
void 
BCMINITFN(_nvram_exit)(void)
{
	nvram_free();
}

/* returns the CRC8 of the nvram */
uint8
BCMINITFN(nvram_calc_crc)(struct nvram_header * nvh)
{
	struct nvram_header tmp;
	uint8 crc;

	/* Little-endian CRC8 over the last 11 bytes of the header */
	tmp.crc_ver_init = htol32((nvh->crc_ver_init & NVRAM_CRC_VER_MASK));
	tmp.config_refresh = htol32(nvh->config_refresh);
	tmp.config_ncdl = htol32(nvh->config_ncdl);

	crc = hndcrc8((uint8 *) &tmp + NVRAM_CRC_START_POSITION,
		sizeof(struct nvram_header) - NVRAM_CRC_START_POSITION,
		CRC8_INIT_VALUE);

	/* Continue CRC8 over data bytes */
	crc = hndcrc8((uint8 *) &nvh[1], nvh->len - sizeof(struct nvram_header), crc);

	return crc;
}

/* Purge un-used value items from the value buffer.
 * Should be locked.
*/
void _nvram_valbuf_compactify(void)
{
	char *wk_buf;
	char *nxt;
	int i;
	int siz;
	struct nvram_dbitem *t, *next;
	int sz = nvram_offset;	//temp

	/* 
	 * KLUDGE!!!  To (try to) avoid a concurrency bug.
	 * Give any reader(s) that just did an nvram_get a little bit of time
	 * to finish using the pointer into the shared mmap'ed value buffer
	 * before we reshuffle the buffer.
	 * A real fix would be to pass back the actual data instead of a pointer,
	 * but that's a lot of rework and testing.  I started that, but ran into
	 * problems---the system wouldn't work. - RVT
	 */
	msleep(700);

	wk_buf = nvram_commit_buf;
	memcpy(wk_buf, nvram_buf, nvram_offset);
	/* Walk all tuples & copy & update value ptrs */
	nxt = nvram_buf;
	for (i = 0; i < NUM_HLH; i++) {
		for (t = nvram_hash[i]; t; t = t->next) {
			if (t->value) {
				siz = strlen(t->value - nvram_buf + wk_buf) +1;
				if (siz + nxt - nvram_buf > NVRAM_VAL_SIZE -4) {
					printk("Oh crap! Over-ran valbuf during compatify. Can't happen!\n");
					printk("We're gonna die!\n");
					break;
				}
				memcpy(nxt, t->value - nvram_buf + wk_buf, siz);
				t->value = nxt;
				nxt += ROUNDUP_P2(siz, 4);
			}
		}
	}

	/* Free dead table */
	for (t = nvram_dead; t; t = next) {
		next = t->next;
		kfree(t);
	}
	nvram_dead = NULL;
	nvram_offset = nxt - nvram_buf;
	printk("compactify:  was: %d   now: %d\n", sz, nvram_offset);	//temp
}

/* In:
 * t = existing node to update its value.
 * If t is NULL, alloc a new node, and set the name & prio.
 * In either case,
 *	If value not NULL or new value is != existing value, then
 *	copy the value param to the nvram_buf.
 * Returns the node.
 */
struct nvram_dbitem *_nvram_realloc(struct nvram_dbitem *t, const char *name, const char *value)
{
	int siz;

	siz = strlen(value) +1;
	if ((nvram_offset + siz) >= NVRAM_VAL_SIZE -256) {
		_nvram_valbuf_compactify();
		if ((nvram_offset + siz) >= NVRAM_VAL_SIZE -256)
			return NULL;
	}

	if (!t) {
		if (!(t = kmalloc(sizeof(struct nvram_tuple) + strlen(name) + 1, GFP_ATOMIC)))
			return NULL;

		strcpy(t->name, name);
		t->prio = prio;
		t->value = NULL;
	}

	/* Copy value. Always on a word boundary.*/
	if (!t->value || strcmp(t->value, value)) {
		t->value = &nvram_buf[nvram_offset];
		memcpy(t->value, value, siz);
		nvram_offset += ROUNDUP_P2(siz, 4);
	}
	return t;
}

void
_nvram_free(struct nvram_dbitem *t)
{
	if (!t)
		nvram_offset = 0;
	else
		kfree(t);
}


/* Return the size the variables will take when written to NVRAM. */
int walk_chain(int z)
{
	int i;
	struct nvram_dbitem *t;

	it_siz = it_cnt = 0;
	for (i = 0; i < NUM_HLH; i++) {
		for (t = nvram_hash[i]; t; t = t->next) {
			it_siz += strlen(t->name) + strlen(t->value) + 2;
			++it_cnt;
			if (z)
				printk("%3d: %s=%-15.15s\n", i,  t->name, t->value);
		}
	}
	return (it_siz);
}

/* For the emacs code formatting
Local Variables:
   c-basic-offset: 8
End:
*/
