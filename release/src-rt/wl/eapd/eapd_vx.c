/*
 * vxWorks-specific portion of EAPD
 * (OS dependent file)
 *
 * Copyright (C) 2010, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: eapd_vx.c 241391 2011-02-18 03:35:48Z stakita $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <signal.h>
#include <ctype.h>

#include <vxWorks.h>
#include <ioLib.h>
#include <ifLib.h>
#include <muxLib.h>
#include <muxTkLib.h>
#include <tickLib.h>
#include <taskLib.h>
#include <errnoLib.h>

#include <typedefs.h>
#include <bcmutils.h>
#include <proto/ethernet.h>
#include <proto/eapol.h>
#include <proto/eap.h>
#include <wlutils.h>
#include <bcmnvram.h>
#include <eapd.h>
#include <shutils.h>
#include <wlif_utils.h>
#include <UdpLib.h>
#include <security_ipc.h>

extern int sysClkRateGet(void);

static eapd_wksp_t *eapd_nwksp = NULL;

#define VX_MUX_MAX_OBJS	EAPD_WKSP_MAX_NO_BRCM + EAPD_WKSP_MAX_NO_BRIDGE
typedef struct vx_mux_obj {
	void		*pCookie;
	int		lbSocket;
	short	lbPort;
	short	used;
} vx_mux_obj_t;
static vx_mux_obj_t muxobjs[VX_MUX_MAX_OBJS];

typedef struct pif2index {
	char		ifname[IFNAMSIZ];
	int		idx;
} pif2index_t;

static pif2index_t if2index[] = {
	{"wl", 1},
	{"et", 2},
	{"vl", 3},
	{"mirror", 4}
};

#ifdef BCMWPA2
#define CHECK_ETHER_TYPE(type) (((type) == ETHER_TYPE_BRCM) || \
	((type) == ETHER_TYPE_802_1X_PREAUTH))
#else
#define CHECK_ETHER_TYPE(type) ((type) == ETHER_TYPE_BRCM)
#endif /* BCMWPA2 */

static void
eapd_hup_hdlr(int sig)
{
	if (eapd_nwksp)
		eapd_nwksp->flags |= EAPD_WKSP_FLAG_SHUTDOWN;

	return;
}

#ifdef EAPDDUMP
static void
eapd_dump_hdlr(int sig)
{
	if (eapd_nwksp)
		eapd_nwksp->flags |= EAPD_WKSP_FLAG_DUMP;

	return;
}
#endif

/* parse interface name and retrieve device name and unit number */
static int
_ifunit_(char *ifname, char *dev, int *unit)
{
	/* Parse unit number */
	for (*dev = *ifname; *dev != EOS && !isdigit((int)*dev); *dev = *ifname) {
		dev++;
		ifname++;
	}
	if (*dev != EOS) {
		*dev = 0;
		*unit = atoi(ifname);
		return OK;
	}

	return ERROR;
}

static int
_get_lbport_(char *dev, int unit, int type)
{
	/* wl, et, vl, mirror */
	int i, base, if2indexsz;

	if2indexsz = sizeof(if2index) / sizeof(pif2index_t);
	base = EAPD_WKSP_NAS_UDP_SPORT + EAPD_WKSP_VX_PORT_OFFSET;

	if (!CHECK_ETHER_TYPE(type))
		return -1;

	for (i = 0; i < if2indexsz; i++) {
		if (!strcmp(if2index[i].ifname, dev))
			break;
	}
	if (i == if2indexsz)
		return -1;

	base += if2index[i].idx;
	base += unit;
	base += (type & 0xff);

	return base;
}

static void
muxobj_init()
{
	int i;

	memset(muxobjs, 0, sizeof(muxobjs));
	for (i = 0; i < VX_MUX_MAX_OBJS; i++)
		muxobjs[i].lbSocket = -1;
}

static vx_mux_obj_t *
muxobj_find(int lbSocket)
{
	int i;

	if (lbSocket < 0)
		return NULL;

	for (i = 0; i < VX_MUX_MAX_OBJS; i++) {
		if ((lbSocket == muxobjs[i].lbSocket) &&
			muxobjs[i].used)
			return (&muxobjs[i]);
	}

	return NULL;
}

static vx_mux_obj_t *
muxobj_get()
{
	int i;

	for (i = 0; i < VX_MUX_MAX_OBJS; i++) {
		if (muxobjs[i].used == 0) {
			muxobjs[i].used = 1;
			return (&muxobjs[i]);
		}
	}

	return NULL;
}

static int
muxobj_free(vx_mux_obj_t *muxobj)
{
	if (muxobj) {
		memset(muxobj, 0, sizeof(vx_mux_obj_t));
		muxobj->lbSocket = -1;
		return 0;
	}

	return -1;
}

/*
* A network service that receives and processes the messages
* sent from wireless END driver (assoc/disassoc) thru MUX.
*/
/* service - shutdown routine - END */
static STATUS
stop_brcm_svc(void *param, void *spare)
{
	return OK;
}

/* send it to loopback socket, eapd receive it in vxWorks platform */
static BOOL
recv_brcm_msg(void *param, long type,
	M_BLK_ID mbuf, LL_HDR_INFO *llhdr, void *spare)
{
	uint8 *pkt = mbuf->mBlkHdr.mData;
	int bytes = mbuf->mBlkHdr.mLen;
	vx_mux_obj_t *muxobj = (vx_mux_obj_t *)spare;

	/* send this date to lbSocket */
	if (muxobj->lbSocket >= 0) {
		int sentBytes = 0;
		struct sockaddr_in to;

		to.sin_addr.s_addr = inet_addr(EAPD_WKSP_UDP_ADDR);
		to.sin_family = AF_INET;
		to.sin_port = htons(muxobj->lbPort);

		sentBytes = sendto(muxobj->lbSocket, pkt, bytes, 0,
			(struct sockaddr *)&to, sizeof(struct sockaddr_in));

		if (sentBytes != bytes) {
			EAPD_ERROR("UDP send failed; sentBytes = %d\n", sentBytes);
		}
		else {
			/* EAPD_ERROR("Send %d bytes to eapd\n", sentBytes); */
		}
	}
	else {
		EAPD_ERROR("eapd vxWorks lbSocket not created\n");
	}

	/* Mesage is ours */
	netMblkClChainFree(mbuf);

	return TRUE;
}

/* service - error notification - END */
static void
notify_brcm_error(END_OBJ *end, END_ERR *error, void *spare)
{
}

/* service - restart - END */
static STATUS
restart_brcm_svc(void *param, void *spare)
{
	return OK;
}

#ifdef BCMWPA2
static STATUS
stop_preauth_svc(void *param, void *spare)
{
	return OK;
}

/* send it to loopback socket, eapd receive it in vxWorks platform */
static BOOL
recv_preauth_msg(void *param, long type, M_BLK_ID mbuf,
	LL_HDR_INFO *llhdr, void *spare)
{
	uint8 *pkt = mbuf->mBlkHdr.mData;
	int bytes = mbuf->mBlkHdr.mLen;
	vx_mux_obj_t *muxobj = (vx_mux_obj_t *)spare;

	/* send this date to lbSocket */
	if (muxobj->lbSocket >= 0) {
		int sentBytes = 0;
		struct sockaddr_in to;

		to.sin_addr.s_addr = inet_addr(EAPD_WKSP_UDP_ADDR);
		to.sin_family = AF_INET;
		to.sin_port = htons(muxobj->lbPort);

		sentBytes = sendto(muxobj->lbSocket, pkt, bytes, 0,
			(struct sockaddr *)&to, sizeof(struct sockaddr_in));

		if (sentBytes != bytes) {
			EAPD_ERROR("UDP send failed; sentBytes = %d\n", sentBytes);
		}
		else {
			/* EAPD_ERROR("Send %d bytes to eapd\n", sentBytes); */
		}
	}
	else {
		EAPD_ERROR("eapd vxWorks lbSocket not created\n");
	}

	/* Mesage is ours */
	netMblkClChainFree(mbuf);

	return TRUE;
}

/* service - error notification - END */
static void
notify_preauth_error(END_OBJ *end, END_ERR *error, void *spare)
{
}

/* service - restart - END */
static STATUS
restart_preauth_svc(void *param, void *spare)
{
	return OK;
}
#endif /* BCMWPA2 */

#include <private/muxLibP.h>
static M_BLK_ID _mblk_(vx_mux_obj_t *muxobj, uint8 *pkt, int len)
{
	END_OBJ *end;
	M_BLK_ID m;

	/* size requested should fit in our cluster buffer */
	if (len >= 1900)
	{
		EAPD_ERROR("packet is too big %d on drvSocket %d\n", len, muxobj->lbSocket);
		return NULL;
	}

	/* muxobj->pCookie is a mux cookie */
	end = PCOOKIE_TO_ENDOBJ(muxobj->pCookie);

	/* alloc packet from pool and copy data */
	if ((m = netTupleGet(end->pNetPool, len, M_DONTWAIT, MT_DATA, FALSE)))
	{
		/* reserve a few bytes */
		m->mBlkHdr.mLen = len;

		/* ensure the cookie field is cleared */
		m->mBlkPktHdr.len = 0;

		/* copy packet content */
		bcopy(pkt, m->mBlkHdr.mData, len);
	}
	else {
		EAPD_ERROR("netTupleGet error 0x%x, end = 0x%x end->pNetPool = 0x%x, pCookie="
			"0x%x\n", errno, (int)end, (int)end->pNetPool, (int)muxobj->pCookie);
	}

	return m;
}

static int
eapd_send(eapd_wksp_t *nwksp, int drvSocket, struct iovec *frags, int nfrags)
{
	struct mbuf *mbuf;
	int i, count;
	char *buf;
	STATUS status;
	vx_mux_obj_t *muxobj;

	/* find muxobj from drvSocket */
	if ((muxobj = muxobj_find(drvSocket)) == NULL) {
		EAPD_ERROR("can not find muxobj from drvSocket %d\n", drvSocket);
		return -1;
	}

	/* Convert iov to mbuf chain */
	if (nfrags > 1) {
		for (i = 0, count = 0; i < nfrags; i++)
			count += frags[i].iov_len;
		if (!(buf = malloc(count))) {
			EAPD_ERROR("malloc error on drvSocket %d\n", drvSocket);
			return errno;
		}
		for (i = 0, count = 0; i < nfrags; i++) {
			memcpy(&buf[count], frags[i].iov_base, frags[i].iov_len);
			count += frags[i].iov_len;
		}
		mbuf = _mblk_(muxobj, (void *)buf, count);
		free(buf);
	}
	else if (nfrags == 1) {
		mbuf = _mblk_(muxobj, (void *)frags[0].iov_base, frags[0].iov_len);
	}
	else {
		EAPD_ERROR("nfrags == 0 error on drvSocket %d\n", drvSocket);
		return EINVAL;
	}

	if (!mbuf) {
		EAPD_ERROR("failed to allocate mblk on drvSocket %d\n", drvSocket);
		return ERROR;
	}

	/* send packet to network thru the interface */
	status = muxSend(muxobj->pCookie, mbuf);
	if (status == END_ERR_BLOCK) {
		EAPD_ERROR("send error %d to drvSocket %d\n", errno, drvSocket);
		netMblkClChainFree(mbuf);
	}
	return status;
}

/* Send a canned EAPOL packet */
void
eapd_eapol_canned_send(eapd_wksp_t *nwksp, struct eapd_socket *Socket, eapd_sta_t *sta,
                                                    unsigned char code, unsigned char type)
{
	eapol_header_t eapol;
	eap_header_t eap;
	struct iovec frags[2];

	memcpy(&eapol.eth.ether_dhost, &sta->ea, ETHER_ADDR_LEN);
	memcpy(&eapol.eth.ether_shost, &sta->bssid, ETHER_ADDR_LEN);

	eapol.eth.ether_type = htons(ETHER_TYPE_802_1X);
	eapol.version = sta->eapol_version;
	eapol.type = EAP_PACKET;
	eapol.length = htons(type ? (EAP_HEADER_LEN + 1) : EAP_HEADER_LEN);

	eap.code = code;
	eap.id = sta->pae_id;
	eap.length = eapol.length;
	eap.type = type;

	frags[0].iov_base = (caddr_t) &eapol;
	frags[0].iov_len = EAPOL_HEADER_LEN;
	frags[1].iov_base = (caddr_t) &eap;
	frags[1].iov_len = ntohs(eapol.length);

	eapd_send(nwksp, Socket->drvSocket, frags, 2);
}

void
eapd_message_send(eapd_wksp_t *nwksp, struct eapd_socket *Socket, uint8 *pData, int pLen)
{
	struct iovec frags[1];

	frags[0].iov_base = (caddr_t) pData;
	frags[0].iov_len = pLen;

	eapd_send(nwksp, Socket->drvSocket, frags, 1);
}

int
eapd_brcm_open(eapd_wksp_t *nwksp, eapd_brcm_socket_t *sock)
{
	int unit, lbport, reuse = 1;
	char dev[16];
	vx_mux_obj_t *muxobj;
	struct sockaddr_in addr;


	if (nwksp == NULL || sock == NULL) {
		EAPD_ERROR("Wrong arguments...\n");
		return -1;
	}

	/* retrieve interface dev name and index */
	_ifunit_(sock->ifname, dev, &unit);

	lbport = _get_lbport_(dev, unit, ETHER_TYPE_BRCM);
	if (lbport == -1) {
		EAPD_ERROR("%s: can not get a loopback bind port ...\n", sock->ifname);
		return -1;
	}

	/* check if the dev is an NPT/END complaint driver */
	if (muxTkDrvCheck(dev) != 0) {
		EAPD_ERROR("%s: not an NTP/END complaint driver ...\n", dev);
		return -1;
	}

	/* open a udp loopback for this ifname and type */
	sock->drvSocket  = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock->drvSocket < 0) {
		EAPD_ERROR("UDP Open failed.\n");
		return -1;
	}
	if (setsockopt(sock->drvSocket, SOL_SOCKET, SO_REUSEPORT, (char*)&reuse,
		sizeof(reuse)) < 0) {
		EAPD_ERROR("UDP setsockopt failed.\n");
		close(sock->drvSocket);
		sock->drvSocket = -1;
		return -1;
	}

	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(lbport);
	if (bind(sock->drvSocket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		EAPD_ERROR("UDP Bind failed, close brcm lbSocket %d\n", sock->drvSocket);
		close(sock->drvSocket);
		sock->drvSocket = -1;
		return -1;
	}
	EAPD_INFO("%s: BRCM socket %d opened\n", sock->ifname, sock->drvSocket);

	if ((muxobj = muxobj_get()) == NULL) {
		EAPD_ERROR("%s: can not get a local muxobj ...\n", sock->ifname);
		close(sock->drvSocket);
		sock->drvSocket = -1;
		return -1;
	}

	/* bind the service to the END driver */
	if (!(muxobj->pCookie = muxBind(dev, unit,
		recv_brcm_msg, stop_brcm_svc,
		restart_brcm_svc, notify_brcm_error,
		ETHER_TYPE_BRCM, "BRCMEVT", muxobj))) {
		EAPD_ERROR("%s: failed to open brcm muxsocket (%x)\n", sock->ifname, errnoGet());
		close(sock->drvSocket);
		sock->drvSocket = -1;
		muxobj_free(muxobj);
		return -1;
	}

	/* at least one use it */
	sock->inuseCount = 1;
	muxobj->lbSocket = sock->drvSocket;
	muxobj->lbPort = lbport;

	EAPD_INFO("%s: BRCM muxsocket %08x opened\n", sock->ifname, (int)muxobj->pCookie);

	return 0;
}

int
eapd_brcm_close(int drvSocket)
{
	int ret;
	vx_mux_obj_t *muxobj;

	if (drvSocket < 0)
		return -1;

	muxobj = muxobj_find(drvSocket);
	if (muxobj) {
		ret = muxUnbind((void *)muxobj->pCookie, ETHER_TYPE_BRCM, recv_brcm_msg);
		if (ret) {
			EAPD_ERROR("brcm close failed 0x%x, drvSocket %d muxsocket %08x\n",
				errno, drvSocket, (int)muxobj->pCookie);
		}
		muxobj_free(muxobj);
	}
	else {
		EAPD_ERROR("muxobj not found drvSocket %d\n", drvSocket);
	}

	/* close lbsocket */
	close(drvSocket);

	return 0;
}

#ifdef BCMWPA2
int
eapd_preauth_open(eapd_wksp_t *nwksp, eapd_preauth_socket_t *sock)
{
	int unit, lbport, reuse = 1;
	char dev[16];
	vx_mux_obj_t *muxobj;
	struct sockaddr_in addr;


	if (nwksp == NULL || sock == NULL) {
		EAPD_ERROR("Wrong arguments...\n");
		return -1;
	}

	/* retrieve interface dev name and index */
	_ifunit_(sock->ifname, dev, &unit);

	lbport = _get_lbport_(dev, unit, ETHER_TYPE_802_1X_PREAUTH);
	if (lbport == -1) {
		EAPD_ERROR("%s: can not get a loopback bind port ...\n", sock->ifname);
		return -1;
	}

	/* check if the dev is an NPT/END complaint driver */
	if (muxTkDrvCheck(dev) != 0) {
		EAPD_ERROR("%s: not an NTP/END complaint driver ...\n", dev);
		return -1;
	}

	/* open a udp loopback for this ifname and type */
	sock->drvSocket  = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock->drvSocket < 0) {
		EAPD_ERROR("UDP Open failed.\n");
		return -1;
	}
	if (setsockopt(sock->drvSocket, SOL_SOCKET, SO_REUSEPORT, (char*)&reuse,
		sizeof(reuse)) < 0) {
		EAPD_ERROR("UDP setsockopt failed.\n");
		close(sock->drvSocket);
		sock->drvSocket = -1;
		return -1;
	}

	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(lbport);
	if (bind(sock->drvSocket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		EAPD_ERROR("UDP Bind failed, close brcm lbSocket %d\n", sock->drvSocket);
		close(sock->drvSocket);
		sock->drvSocket = -1;
		return -1;
	}
	EAPD_INFO("%s: preauth socket %d opened\n", sock->ifname, sock->drvSocket);

	if ((muxobj = muxobj_get()) == NULL) {
		EAPD_ERROR("%s: can not get a local muxobj ...\n", sock->ifname);
		close(sock->drvSocket);
		sock->drvSocket = -1;
		return -1;
	}

	/* bind the service to the END driver */
	if (!(muxobj->pCookie = muxBind(dev, unit,
		recv_preauth_msg, stop_preauth_svc,
		restart_preauth_svc, notify_preauth_error,
		ETHER_TYPE_802_1X_PREAUTH, "PREAUTH", muxobj))) {
		EAPD_ERROR("%s: failed to open preauth muxsocket (%x)\n", sock->ifname, errnoGet());
		close(sock->drvSocket);
		sock->drvSocket = -1;
		muxobj_free(muxobj);
		return -1;
	}

	/* at least one use it */
	sock->inuseCount = 1;
	muxobj->lbSocket = sock->drvSocket;
	muxobj->lbPort = lbport;

	EAPD_INFO("%s: preauth muxsocket %08x opened\n", sock->ifname, (int)muxobj->pCookie);

	return 0;
}

int
eapd_preauth_close(int drvSocket)
{
	int ret;
	vx_mux_obj_t *muxobj;

	if (drvSocket < 0)
		return -1;

	muxobj = muxobj_find(drvSocket);
	if (muxobj) {
		ret = muxUnbind((void *)muxobj->pCookie, ETHER_TYPE_802_1X_PREAUTH,
			recv_preauth_msg);
		if (ret) {
			EAPD_ERROR("preauth close failed 0x%x, drvSocket %d muxsocket %08x\n",
				errno, drvSocket, (int)muxobj->pCookie);
		}
		muxobj_free(muxobj);
	}
	else {
		EAPD_ERROR("muxobj not found drvSocket %d\n", drvSocket);
	}

	/* close lbsocket */
	close(drvSocket);

	return 0;
}
#endif /* BCMWPA2 */

/*
 * Configuration APIs
 */
int
eapd_safe_get_conf(char *outval, int outval_size, char *name)
{
	char *val;

	if (name == NULL || outval == NULL) {
		if (outval)
			memset(outval, 0, outval_size);
		return -1;
	}

	val = nvram_safe_get(name);
	if (!strcmp(val, ""))
		memset(outval, 0, outval_size);
	else
		snprintf(outval, outval_size, "%s", val);
	return 0;
}

int eapd_main(int argc, char* argv[])
{
#ifdef BCMDBG
	char *dbg;
#endif

#ifdef BCMDBG
	/* get eapd_msg_level from nvram */
	if ((dbg = nvram_get("eapd_dbg"))) {
		eapd_msg_level = (uint)strtoul(dbg, NULL, 0);
	}

#endif

	EAPD_INFO("EAP Dispatch Start...\n");

	/* clear muxobjs array */
	muxobj_init();

	/* alloc eapd work space */
	if (!(eapd_nwksp = eapd_wksp_alloc_workspace())) {
		EAPD_ERROR("Unable to allocate wksp memory. Quitting...\n");
		return -1;
	}

	if (eapd_wksp_auto_config(eapd_nwksp)) {
		EAPD_ERROR("Unable to auto config. Quitting...\n");
		eapd_wksp_cleanup(eapd_nwksp);
		return -1;
	}

	/* establish a handler to handle SIGTERM. */
	signal(SIGTERM, eapd_hup_hdlr);

#ifdef EAPDDUMP
	signal(SIGUSR1, eapd_dump_hdlr);
#endif

	eapd_wksp_main_loop(eapd_nwksp);

	EAPD_INFO("EAP Dispatcher Stopped...\n");

	return 0;

}

void
eapdStart(void)
{
	int tid = taskNameToId("EAPD");
	ULONG ticks;

	if (tid == ERROR) {
		/* clear eapd wksp initialization flag */
		eapd_wksp_clear_inited();

		taskSpawn("EAPD",
			 60, /* priority of new task */
			 0, /* task option word */
			 30000,  /* size (bytes) of stack needed plus name */
			 (FUNCPTR)eapd_main,   /* entry point of new task */
			 0,
			 0,
			 0, 0, 0, 0, 0, 0, 0, 0);
		printf("EAPD task started.\n");

		/* wait until eapd initialization finished */
		ticks = tickGet();
		do {
			if (tickGet() - ticks < 3 * sysClkRateGet())
				taskDelay(sysClkRateGet());
			else {
				printf("Unable to wait EAPD initialization finished!.\n");
				return;
			}
		} while (taskNameToId("EAPD") != ERROR && !eapd_wksp_is_inited());
	}
	else
		printf("EAPD task is already running.\n");
}

void
eapdStop(void)
{
	int tid = taskNameToId("EAPD");

	if (tid != ERROR) {
		ULONG ticks;

		kill(tid, SIGTERM);

		/* wait till the task is dead */
		ticks = tickGet();
		do {
			if (tickGet() - ticks < 3 * sysClkRateGet())
				taskDelay(sysClkRateGet());
			else {
				printf("Unable to kill EAPD task!.\n");
				return;
			}
		}
		while (taskNameToId("EAPD") != ERROR);
		printf("EAPD task killed.\n");
	}
	else
		printf("EAPD task is not running.\n");

}
