/*
 * ioctl32.c: Conversion between 32bit and 64bit native ioctls.
 *
 * Copyright (C) 2000 Silicon Graphics, Inc.
 * Written by Ulf Carlsson (ulfc@engr.sgi.com)
 * Copyright (C) 2000 Ralf Baechle
 * Copyright (C) 2002  Maciej W. Rozycki
 *
 * Mostly stolen from the sparc64 ioctl32 implementation.
 */
#include <linux/config.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/if.h>
#include <linux/mm.h>
#include <linux/mtio.h>
#include <linux/init.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/ppp_defs.h>
#include <linux/if_ppp.h>
#include <linux/if_pppox.h>
#include <linux/cdrom.h>
#include <linux/loop.h>
#include <linux/fb.h>
#include <linux/vt.h>
#include <linux/kd.h>
#include <linux/netdevice.h>
#include <linux/kernprof.h>
#include <linux/route.h>
#include <linux/hdreg.h>
#include <linux/blkpg.h>
#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/auto_fs.h>
#include <linux/auto_fs4.h>
#include <linux/ext2_fs.h>
#include <linux/raid/md_u.h>

#include <scsi/scsi.h>
#undef __KERNEL__		/* This file was born to be ugly ...  */
#include <scsi/scsi_ioctl.h>
#define __KERNEL__
#include <scsi/sg.h>

#include <asm/types.h>
#include <asm/uaccess.h>

#include <linux/rtc.h>

long sys_ioctl(unsigned int fd, unsigned int cmd, unsigned long arg);

static int w_long(unsigned int fd, unsigned int cmd, unsigned long arg)
{
	mm_segment_t old_fs = get_fs();
	int err;
	unsigned long val;

	set_fs (KERNEL_DS);
	err = sys_ioctl(fd, cmd, (unsigned long)&val);
	set_fs (old_fs);
	if (!err && put_user((unsigned int) val, (u32 *)arg))
		return -EFAULT;
	return err;
}

static int rw_long(unsigned int fd, unsigned int cmd, unsigned long arg)
{
	mm_segment_t old_fs = get_fs();
	int err;
	unsigned long val;

	if (get_user(val, (u32 *)arg))
		return -EFAULT;
	set_fs(KERNEL_DS);
	err = sys_ioctl(fd, cmd, (unsigned long)&val);
	set_fs (old_fs);
	if (!err && put_user(val, (u32 *)arg))
		return -EFAULT;
	return err;
}

#define A(__x) ((unsigned long)(__x))


#ifdef CONFIG_FB

struct fb_fix_screeninfo32 {
	char id[16];			/* identification string eg "TT Builtin" */
	__u32 smem_start;		/* Start of frame buffer mem */
					/* (physical address) */
	__u32 smem_len;			/* Length of frame buffer mem */
	__u32 type;			/* see FB_TYPE_*		*/
	__u32 type_aux;			/* Interleave for interleaved Planes */
	__u32 visual;			/* see FB_VISUAL_*		*/ 
	__u16 xpanstep;			/* zero if no hardware panning  */
	__u16 ypanstep;			/* zero if no hardware panning  */
	__u16 ywrapstep;		/* zero if no hardware ywrap    */
	__u32 line_length;		/* length of a line in bytes    */
	__u32 mmio_start;		/* Start of Memory Mapped I/O   */
					/* (physical address) */
	__u32 mmio_len;			/* Length of Memory Mapped I/O  */
	__u32 accel;			/* Type of acceleration available */
	__u16 reserved[3];		/* Reserved for future compatibility */
};

static int do_fbioget_fscreeninfo_ioctl(unsigned int fd, unsigned int cmd,
					unsigned long arg)
{
	mm_segment_t old_fs = get_fs();
	struct fb_fix_screeninfo fix;
	struct fb_fix_screeninfo32 *fix32 = (struct fb_fix_screeninfo32 *)arg;
	int err;

	set_fs(KERNEL_DS);
	err = sys_ioctl(fd, cmd, (unsigned long)&fix);
	set_fs(old_fs);

	if (err == 0) {
		err = __copy_to_user((char *)fix32->id, (char *)fix.id,
				     sizeof(fix.id));
		err |= __put_user((__u32)(unsigned long)fix.smem_start,
				  &fix32->smem_start);
		err |= __put_user(fix.smem_len, &fix32->smem_len);
		err |= __put_user(fix.type, &fix32->type);
		err |= __put_user(fix.type_aux, &fix32->type_aux);
		err |= __put_user(fix.visual, &fix32->visual);
		err |= __put_user(fix.xpanstep, &fix32->xpanstep);
		err |= __put_user(fix.ypanstep, &fix32->ypanstep);
		err |= __put_user(fix.ywrapstep, &fix32->ywrapstep);
		err |= __put_user(fix.line_length, &fix32->line_length);
		err |= __put_user((__u32)(unsigned long)fix.mmio_start,
				  &fix32->mmio_start);
		err |= __put_user(fix.mmio_len, &fix32->mmio_len);
		err |= __put_user(fix.accel, &fix32->accel);
		err |= __copy_to_user((char *)fix32->reserved,
				      (char *)fix.reserved,
				      sizeof(fix.reserved));
		if (err)
			err = -EFAULT;
	}

	return err;
}

struct fb_cmap32 {
	__u32 start;			/* First entry  */
	__u32 len;			/* Number of entries */
	__u32 red;			/* Red values   */
	__u32 green;
	__u32 blue;
	__u32 transp;			/* transparency, can be NULL */
};

static int do_fbiocmap_ioctl(unsigned int fd, unsigned int cmd,
			     unsigned long arg)
{
	mm_segment_t old_fs = get_fs();
	u32 red = 0, green = 0, blue = 0, transp = 0;
	struct fb_cmap cmap;
	struct fb_cmap32 *cmap32 = (struct fb_cmap32 *)arg;
	int err;

	memset(&cmap, 0, sizeof(cmap));

	err = __get_user(cmap.start, &cmap32->start);
	err |= __get_user(cmap.len, &cmap32->len);
	err |= __get_user(red, &cmap32->red);
	err |= __get_user(green, &cmap32->green);
	err |= __get_user(blue, &cmap32->blue);
	err |= __get_user(transp, &cmap32->transp);
	if (err)
		return -EFAULT;

	err = -ENOMEM;
	cmap.red = kmalloc(cmap.len * sizeof(__u16), GFP_KERNEL);
	if (!cmap.red)
		goto out;
	cmap.green = kmalloc(cmap.len * sizeof(__u16), GFP_KERNEL);
	if (!cmap.green)
		goto out;
	cmap.blue = kmalloc(cmap.len * sizeof(__u16), GFP_KERNEL);
	if (!cmap.blue)
		goto out;
	if (transp) {
		cmap.transp = kmalloc(cmap.len * sizeof(__u16), GFP_KERNEL);
		if (!cmap.transp)
			goto out;
	}
			
	if (cmd == FBIOPUTCMAP) {
		err = __copy_from_user(cmap.red, (char *)A(red),
				       cmap.len * sizeof(__u16));
		err |= __copy_from_user(cmap.green, (char *)A(green),
					cmap.len * sizeof(__u16));
		err |= __copy_from_user(cmap.blue, (char *)A(blue),
					cmap.len * sizeof(__u16));
		if (cmap.transp)
			err |= __copy_from_user(cmap.transp, (char *)A(transp),
						cmap.len * sizeof(__u16));
		if (err) {
			err = -EFAULT;
			goto out;
		}
	}

	set_fs(KERNEL_DS);
	err = sys_ioctl(fd, cmd, (unsigned long)&cmap);
	set_fs(old_fs);
	if (err)
		goto out;

	if (cmd == FBIOGETCMAP) {
		err = __copy_to_user((char *)A(red), cmap.red,
				     cmap.len * sizeof(__u16));
		err |= __copy_to_user((char *)A(green), cmap.blue,
				      cmap.len * sizeof(__u16));
		err |= __copy_to_user((char *)A(blue), cmap.blue,
				      cmap.len * sizeof(__u16));
		if (cmap.transp)
			err |= __copy_to_user((char *)A(transp), cmap.transp,
					      cmap.len * sizeof(__u16));
		if (err) {
			err = -EFAULT;
			goto out;
		}
	}

out:
	if (cmap.red)
		kfree(cmap.red);
	if (cmap.green)
		kfree(cmap.green);
	if (cmap.blue)
		kfree(cmap.blue);
	if (cmap.transp)
		kfree(cmap.transp);

	return err;
}

#endif /* CONFIG_FB */


struct timeval32 {
	int tv_sec;
	int tv_usec;
};

static int do_siocgstamp(unsigned int fd, unsigned int cmd, unsigned long arg)
{
	struct timeval32 *up = (struct timeval32 *)arg;
	struct timeval ktv;
	mm_segment_t old_fs = get_fs();
	int err;

	set_fs(KERNEL_DS);
	err = sys_ioctl(fd, cmd, (unsigned long)&ktv);
	set_fs(old_fs);
	if (!err) {
		err = put_user(ktv.tv_sec, &up->tv_sec);
		err |= __put_user(ktv.tv_usec, &up->tv_usec);
	}

	return err;
}

#define EXT2_IOC32_GETFLAGS               _IOR('f', 1, int)
#define EXT2_IOC32_SETFLAGS               _IOW('f', 2, int)
#define EXT2_IOC32_GETVERSION             _IOR('v', 1, int)
#define EXT2_IOC32_SETVERSION             _IOW('v', 2, int)

struct ifmap32 {
	unsigned int mem_start;
	unsigned int mem_end;
	unsigned short base_addr;
	unsigned char irq;
	unsigned char dma;
	unsigned char port;
};

struct ifreq32 {
#define IFHWADDRLEN     6
#define IFNAMSIZ        16
        union {
                char    ifrn_name[IFNAMSIZ];	/* if name, e.g. "en0" */
        } ifr_ifrn;
        union {
                struct  sockaddr ifru_addr;
                struct  sockaddr ifru_dstaddr;
                struct  sockaddr ifru_broadaddr;
                struct  sockaddr ifru_netmask;
                struct  sockaddr ifru_hwaddr;
                short   ifru_flags;
                int     ifru_ivalue;
                int     ifru_mtu;
                struct  ifmap32 ifru_map;
                char    ifru_slave[IFNAMSIZ];   /* Just fits the size */
		char	ifru_newname[IFNAMSIZ];
                __kernel_caddr_t32 ifru_data;
        } ifr_ifru;
};

struct ifconf32 {
        int     ifc_len;                        /* size of buffer       */
        __kernel_caddr_t32  ifcbuf;
};

#ifdef CONFIG_NET

static int dev_ifname32(unsigned int fd, unsigned int cmd, unsigned long arg)
{
	struct ireq32 *uir32 = (struct ireq32 *)arg;
	struct net_device *dev;
	struct ifreq32 ifr32;

	if (copy_from_user(&ifr32, uir32, sizeof(struct ifreq32)))
		return -EFAULT;

	read_lock(&dev_base_lock);
	dev = __dev_get_by_index(ifr32.ifr_ifindex);
	if (!dev) {
		read_unlock(&dev_base_lock);
		return -ENODEV;
	}

	strcpy(ifr32.ifr_name, dev->name);
	read_unlock(&dev_base_lock);

	if (copy_to_user(uir32, &ifr32, sizeof(struct ifreq32)))
	    return -EFAULT;

	return 0;
}

static inline int dev_ifconf(unsigned int fd, unsigned int cmd,
			     unsigned long arg)
{
	struct ioconf32 *uifc32 = (struct ioconf32 *)arg;
	struct ifconf32 ifc32;
	struct ifconf ifc;
	struct ifreq32 *ifr32;
	struct ifreq *ifr;
	mm_segment_t old_fs;
	int len;
	int err;

	if (copy_from_user(&ifc32, uifc32, sizeof(struct ifconf32)))
		return -EFAULT;

	if(ifc32.ifcbuf == 0) {
		ifc32.ifc_len = 0;
		ifc.ifc_len = 0;
		ifc.ifc_buf = NULL;
	} else {
		ifc.ifc_len = ((ifc32.ifc_len / sizeof (struct ifreq32))) *
			sizeof (struct ifreq);
		ifc.ifc_buf = kmalloc (ifc.ifc_len, GFP_KERNEL);
		if (!ifc.ifc_buf)
			return -ENOMEM;
	}
	ifr = ifc.ifc_req;
	ifr32 = (struct ifreq32 *)A(ifc32.ifcbuf);
	len = ifc32.ifc_len / sizeof (struct ifreq32);
	while (len--) {
		if (copy_from_user(ifr++, ifr32++, sizeof (struct ifreq32))) {
			err = -EFAULT;
			goto out;
		}
	}

	old_fs = get_fs();
	set_fs (KERNEL_DS);
	err = sys_ioctl (fd, SIOCGIFCONF, (unsigned long)&ifc);
	set_fs (old_fs);
	if (err)
		goto out;

	ifr = ifc.ifc_req;
	ifr32 = (struct ifreq32 *)A(ifc32.ifcbuf);
	len = ifc.ifc_len / sizeof (struct ifreq);
	ifc32.ifc_len = len * sizeof (struct ifreq32);

	while (len--) {
		if (copy_to_user(ifr32++, ifr++, sizeof (struct ifreq32))) {
			err = -EFAULT;
			goto out;
		}
	}

	if (copy_to_user(uifc32, &ifc32, sizeof(struct ifconf32))) {
		err = -EFAULT;
		goto out;
	}
out:
	if(ifc.ifc_buf != NULL)
		kfree (ifc.ifc_buf);
	return err;
}

static int dev_ifsioc(unsigned int fd, unsigned int cmd, unsigned long arg)
{
	struct ifreq ifr;
	mm_segment_t old_fs;
	int err;
	
	switch (cmd) {
	case SIOCSIFMAP:
		err = copy_from_user(&ifr, (struct ifreq32 *)arg, sizeof(ifr.ifr_name));
		err |= __get_user(ifr.ifr_map.mem_start, &(((struct ifreq32 *)arg)->ifr_ifru.ifru_map.mem_start));
		err |= __get_user(ifr.ifr_map.mem_end, &(((struct ifreq32 *)arg)->ifr_ifru.ifru_map.mem_end));
		err |= __get_user(ifr.ifr_map.base_addr, &(((struct ifreq32 *)arg)->ifr_ifru.ifru_map.base_addr));
		err |= __get_user(ifr.ifr_map.irq, &(((struct ifreq32 *)arg)->ifr_ifru.ifru_map.irq));
		err |= __get_user(ifr.ifr_map.dma, &(((struct ifreq32 *)arg)->ifr_ifru.ifru_map.dma));
		err |= __get_user(ifr.ifr_map.port, &(((struct ifreq32 *)arg)->ifr_ifru.ifru_map.port));
		if (err)
			return -EFAULT;
		break;
	default:
		if (copy_from_user(&ifr, (struct ifreq32 *)arg, sizeof(struct ifreq32)))
			return -EFAULT;
		break;
	}
	old_fs = get_fs();
	set_fs (KERNEL_DS);
	err = sys_ioctl (fd, cmd, (unsigned long)&ifr);
	set_fs (old_fs);
	if (!err) {
		switch (cmd) {
		case SIOCGIFFLAGS:
		case SIOCGIFMETRIC:
		case SIOCGIFMTU:
		case SIOCGIFMEM:
		case SIOCGIFHWADDR:
		case SIOCGIFINDEX:
		case SIOCGIFADDR:
		case SIOCGIFBRDADDR:
		case SIOCGIFDSTADDR:
		case SIOCGIFNETMASK:
		case SIOCGIFTXQLEN:
			if (copy_to_user((struct ifreq32 *)arg, &ifr, sizeof(struct ifreq32)))
				return -EFAULT;
			break;
		case SIOCGIFMAP:
			err = copy_to_user((struct ifreq32 *)arg, &ifr, sizeof(ifr.ifr_name));
			err |= __put_user(ifr.ifr_map.mem_start, &(((struct ifreq32 *)arg)->ifr_ifru.ifru_map.mem_start));
			err |= __put_user(ifr.ifr_map.mem_end, &(((struct ifreq32 *)arg)->ifr_ifru.ifru_map.mem_end));
			err |= __put_user(ifr.ifr_map.base_addr, &(((struct ifreq32 *)arg)->ifr_ifru.ifru_map.base_addr));
			err |= __put_user(ifr.ifr_map.irq, &(((struct ifreq32 *)arg)->ifr_ifru.ifru_map.irq));
			err |= __put_user(ifr.ifr_map.dma, &(((struct ifreq32 *)arg)->ifr_ifru.ifru_map.dma));
			err |= __put_user(ifr.ifr_map.port, &(((struct ifreq32 *)arg)->ifr_ifru.ifru_map.port));
			if (err)
				err = -EFAULT;
			break;
		}
	}
	return err;
}

struct rtentry32
{
	unsigned int	rt_pad1;
	struct sockaddr	rt_dst;		/* target address		*/
	struct sockaddr	rt_gateway;	/* gateway addr (RTF_GATEWAY)	*/
	struct sockaddr	rt_genmask;	/* target network mask (IP)	*/
	unsigned short	rt_flags;
	short		rt_pad2;
	unsigned int	rt_pad3;
	unsigned int	rt_pad4;
	short		rt_metric;	/* +1 for binary compatibility!	*/
	unsigned int	rt_dev;		/* forcing the device at add	*/
	unsigned int	rt_mtu;		/* per route MTU/Window 	*/
#ifndef __KERNEL__
#define rt_mss	rt_mtu			/* Compatibility :-(            */
#endif
	unsigned int	rt_window;	/* Window clamping 		*/
	unsigned short	rt_irtt;	/* Initial RTT			*/
};

static inline int routing_ioctl(unsigned int fd, unsigned int cmd, unsigned long arg)
{
	struct rtentry32 *ur = (struct rtentry32 *)arg;
	struct rtentry r;
	char devname[16];
	u32 rtdev;
	int ret;
	mm_segment_t old_fs = get_fs();

	ret = copy_from_user (&r.rt_dst, &(ur->rt_dst), 3 * sizeof(struct sockaddr));
	ret |= __get_user (r.rt_flags, &(ur->rt_flags));
	ret |= __get_user (r.rt_metric, &(ur->rt_metric));
	ret |= __get_user (r.rt_mtu, &(ur->rt_mtu));
	ret |= __get_user (r.rt_window, &(ur->rt_window));
	ret |= __get_user (r.rt_irtt, &(ur->rt_irtt));
	ret |= __get_user (rtdev, &(ur->rt_dev));
	if (rtdev) {
		ret |= copy_from_user (devname, (char *)A(rtdev), 15);
		r.rt_dev = devname; devname[15] = 0;
	} else
		r.rt_dev = 0;
	if (ret)
		return -EFAULT;
	set_fs (KERNEL_DS);
	ret = sys_ioctl (fd, cmd, (long)&r);
	set_fs (old_fs);
	return ret;
}

#endif /* CONFIG_NET */

static int do_ext2_ioctl(unsigned int fd, unsigned int cmd, unsigned long arg)
{
	/* These are just misnamed, they actually get/put from/to user an int */
	switch (cmd) {
	case EXT2_IOC32_GETFLAGS: cmd = EXT2_IOC_GETFLAGS; break;
	case EXT2_IOC32_SETFLAGS: cmd = EXT2_IOC_SETFLAGS; break;
	case EXT2_IOC32_GETVERSION: cmd = EXT2_IOC_GETVERSION; break;
	case EXT2_IOC32_SETVERSION: cmd = EXT2_IOC_SETVERSION; break;
	}
	return sys_ioctl(fd, cmd, arg);
}

struct hd_geometry32 {
	unsigned char heads;
	unsigned char sectors;
	unsigned short cylinders;
	u32 start;
};

static int hdio_getgeo(unsigned int fd, unsigned int cmd, unsigned long arg)
{
	mm_segment_t old_fs = get_fs();
	struct hd_geometry geo;
	int err;

	set_fs (KERNEL_DS);
	err = sys_ioctl(fd, HDIO_GETGEO, (unsigned long)&geo);
	set_fs (old_fs);
	if (!err) {
		err = copy_to_user ((struct hd_geometry32 *)arg, &geo, 4);
		err |= __put_user (geo.start, &(((struct hd_geometry32 *)arg)->start));
	}

	return err ? -EFAULT : 0;
}

static int hdio_ioctl_trans(unsigned int fd, unsigned int cmd, unsigned long arg)
{
	mm_segment_t old_fs = get_fs();
	unsigned long kval;
	unsigned int *uvp;
	int error;

	set_fs(KERNEL_DS);
	error = sys_ioctl(fd, cmd, (long)&kval);
	set_fs(old_fs);

	if (error == 0) {
		uvp = (unsigned int *)arg;
		if (put_user(kval, uvp))
			error = -EFAULT;
	}

	return error;
}

static int ret_einval(unsigned int fd, unsigned int cmd, unsigned long arg)
{
	return -EINVAL;
}

struct blkpg_ioctl_arg32 {
	int op;
	int flags;
	int datalen;
	u32 data;
};

static int blkpg_ioctl_trans(unsigned int fd, unsigned int cmd,
                             struct blkpg_ioctl_arg32 *arg)
{
	struct blkpg_ioctl_arg a;
	struct blkpg_partition p;
	int err;
	mm_segment_t old_fs = get_fs();

	err = get_user(a.op, &arg->op);
	err |= __get_user(a.flags, &arg->flags);
	err |= __get_user(a.datalen, &arg->datalen);
	err |= __get_user((long)a.data, &arg->data);
	if (err) return err;
	switch (a.op) {
	case BLKPG_ADD_PARTITION:
	case BLKPG_DEL_PARTITION:
		if (a.datalen < sizeof(struct blkpg_partition))
			return -EINVAL;
                if (copy_from_user(&p, a.data, sizeof(struct blkpg_partition)))
			return -EFAULT;
		a.data = &p;
		set_fs (KERNEL_DS);
		err = sys_ioctl(fd, cmd, (unsigned long)&a);
		set_fs (old_fs);
	default:
		return -EINVAL;
	}
	return err;
}

struct mtget32 {
	__u32	mt_type;
	__u32	mt_resid;
	__u32	mt_dsreg;
	__u32	mt_gstat;
	__u32	mt_erreg;
	__kernel_daddr_t32	mt_fileno;
	__kernel_daddr_t32	mt_blkno;
};
#define MTIOCGET32	_IOR('m', 2, struct mtget32)

struct mtpos32 {
	__u32	mt_blkno;
};
#define MTIOCPOS32	_IOR('m', 3, struct mtpos32)

struct mtconfiginfo32 {
	__u32	mt_type;
	__u32	ifc_type;
	__u16	irqnr;
	__u16	dmanr;
	__u16	port;
	__u32	debug;
	__u32	have_dens:1;
	__u32	have_bsf:1;
	__u32	have_fsr:1;
	__u32	have_bsr:1;
	__u32	have_eod:1;
	__u32	have_seek:1;
	__u32	have_tell:1;
	__u32	have_ras1:1;
	__u32	have_ras2:1;
	__u32	have_ras3:1;
	__u32	have_qfa:1;
	__u32	pad1:5;
	char	reserved[10];
};
#define	MTIOCGETCONFIG32	_IOR('m', 4, struct mtconfiginfo32)
#define	MTIOCSETCONFIG32	_IOW('m', 5, struct mtconfiginfo32)

static int mt_ioctl_trans(unsigned int fd, unsigned int cmd, unsigned long arg)
{
	mm_segment_t old_fs = get_fs();
	struct mtconfiginfo info;
	struct mtget get;
	struct mtpos pos;
	unsigned long kcmd;
	void *karg;
	int err = 0;

	switch(cmd) {
	case MTIOCPOS32:
		kcmd = MTIOCPOS;
		karg = &pos;
		break;
	case MTIOCGET32:
		kcmd = MTIOCGET;
		karg = &get;
		break;
	case MTIOCGETCONFIG32:
		kcmd = MTIOCGETCONFIG;
		karg = &info;
		break;
	case MTIOCSETCONFIG32:
		kcmd = MTIOCSETCONFIG;
		karg = &info;
		err = __get_user(info.mt_type, &((struct mtconfiginfo32 *)arg)->mt_type);
		err |= __get_user(info.ifc_type, &((struct mtconfiginfo32 *)arg)->ifc_type);
		err |= __get_user(info.irqnr, &((struct mtconfiginfo32 *)arg)->irqnr);
		err |= __get_user(info.dmanr, &((struct mtconfiginfo32 *)arg)->dmanr);
		err |= __get_user(info.port, &((struct mtconfiginfo32 *)arg)->port);
		err |= __get_user(info.debug, &((struct mtconfiginfo32 *)arg)->debug);
		err |= __copy_from_user((char *)&info.debug + sizeof(info.debug),
				     (char *)&((struct mtconfiginfo32 *)arg)->debug
				     + sizeof(((struct mtconfiginfo32 *)arg)->debug), sizeof(__u32));
		if (err)
			return -EFAULT;
		break;
	default:
		do {
			static int count = 0;
			if (++count <= 20)
				printk("mt_ioctl: Unknown cmd fd(%d) "
				       "cmd(%08x) arg(%08x)\n",
				       (int)fd, (unsigned int)cmd, (unsigned int)arg);
		} while(0);
		return -EINVAL;
	}
	set_fs (KERNEL_DS);
	err = sys_ioctl (fd, kcmd, (unsigned long)karg);
	set_fs (old_fs);
	if (err)
		return err;
	switch (cmd) {
	case MTIOCPOS32:
		err = __put_user(pos.mt_blkno, &((struct mtpos32 *)arg)->mt_blkno);
		break;
	case MTIOCGET32:
		err = __put_user(get.mt_type, &((struct mtget32 *)arg)->mt_type);
		err |= __put_user(get.mt_resid, &((struct mtget32 *)arg)->mt_resid);
		err |= __put_user(get.mt_dsreg, &((struct mtget32 *)arg)->mt_dsreg);
		err |= __put_user(get.mt_gstat, &((struct mtget32 *)arg)->mt_gstat);
		err |= __put_user(get.mt_erreg, &((struct mtget32 *)arg)->mt_erreg);
		err |= __put_user(get.mt_fileno, &((struct mtget32 *)arg)->mt_fileno);
		err |= __put_user(get.mt_blkno, &((struct mtget32 *)arg)->mt_blkno);
		break;
	case MTIOCGETCONFIG32:
		err = __put_user(info.mt_type, &((struct mtconfiginfo32 *)arg)->mt_type);
		err |= __put_user(info.ifc_type, &((struct mtconfiginfo32 *)arg)->ifc_type);
		err |= __put_user(info.irqnr, &((struct mtconfiginfo32 *)arg)->irqnr);
		err |= __put_user(info.dmanr, &((struct mtconfiginfo32 *)arg)->dmanr);
		err |= __put_user(info.port, &((struct mtconfiginfo32 *)arg)->port);
		err |= __put_user(info.debug, &((struct mtconfiginfo32 *)arg)->debug);
		err |= __copy_to_user((char *)&((struct mtconfiginfo32 *)arg)->debug
			    		   + sizeof(((struct mtconfiginfo32 *)arg)->debug),
					   (char *)&info.debug + sizeof(info.debug), sizeof(__u32));
		break;
	case MTIOCSETCONFIG32:
		break;
	}
	return err ? -EFAULT: 0;
}

#define AUTOFS_IOC_SETTIMEOUT32 _IOWR(0x93,0x64,unsigned int)

static int ioc_settimeout(unsigned int fd, unsigned int cmd, unsigned long arg)
{
	return rw_long(fd, AUTOFS_IOC_SETTIMEOUT, arg);
}

struct ioctl32_handler {
	unsigned int cmd;
	int (*function)(unsigned int, unsigned int, unsigned long);
};

struct ioctl32_list {
	struct ioctl32_handler handler;
	struct ioctl32_list *next;
};

#define IOCTL32_DEFAULT(cmd)		{ { cmd, (void *) sys_ioctl }, 0 }
#define IOCTL32_HANDLER(cmd, handler)	{ { cmd, (void *) handler }, 0 }

static struct ioctl32_list ioctl32_handler_table[] = {
	IOCTL32_DEFAULT(TCGETA),
	IOCTL32_DEFAULT(TCSETA),
	IOCTL32_DEFAULT(TCSETAW),
	IOCTL32_DEFAULT(TCSETAF),
	IOCTL32_DEFAULT(TCSBRK),
	IOCTL32_DEFAULT(TCXONC),
	IOCTL32_DEFAULT(TCFLSH),
	IOCTL32_DEFAULT(TCGETS),
	IOCTL32_DEFAULT(TCSETS),
	IOCTL32_DEFAULT(TCSETSW),
	IOCTL32_DEFAULT(TCSETSF),
	IOCTL32_DEFAULT(TIOCLINUX),

	IOCTL32_DEFAULT(TIOCGETD),
	IOCTL32_DEFAULT(TIOCSETD),
	IOCTL32_DEFAULT(TIOCEXCL),
	IOCTL32_DEFAULT(TIOCNXCL),
	IOCTL32_DEFAULT(TIOCCONS),
	IOCTL32_DEFAULT(TIOCGSOFTCAR),
	IOCTL32_DEFAULT(TIOCSSOFTCAR),
	IOCTL32_DEFAULT(TIOCSWINSZ),
	IOCTL32_DEFAULT(TIOCGWINSZ),
	IOCTL32_DEFAULT(TIOCMGET),
	IOCTL32_DEFAULT(TIOCMBIC),
	IOCTL32_DEFAULT(TIOCMBIS),
	IOCTL32_DEFAULT(TIOCMSET),
	IOCTL32_DEFAULT(TIOCPKT),
	IOCTL32_DEFAULT(TIOCNOTTY),
	IOCTL32_DEFAULT(TIOCSTI),
	IOCTL32_DEFAULT(TIOCOUTQ),
	IOCTL32_DEFAULT(TIOCSPGRP),
	IOCTL32_DEFAULT(TIOCGPGRP),
	IOCTL32_DEFAULT(TIOCSCTTY),
	IOCTL32_DEFAULT(TIOCGPTN),
	IOCTL32_DEFAULT(TIOCSPTLCK),
	IOCTL32_DEFAULT(TIOCGSERIAL),
	IOCTL32_DEFAULT(TIOCSSERIAL),
	IOCTL32_DEFAULT(TIOCSERGETLSR),

	IOCTL32_DEFAULT(FIOCLEX),
	IOCTL32_DEFAULT(FIONCLEX),
	IOCTL32_DEFAULT(FIOASYNC),
	IOCTL32_DEFAULT(FIONBIO),
	IOCTL32_DEFAULT(FIONREAD),

#ifdef CONFIG_FB
	/* Big F */
	IOCTL32_DEFAULT(FBIOGET_VSCREENINFO),
	IOCTL32_DEFAULT(FBIOPUT_VSCREENINFO),
	IOCTL32_HANDLER(FBIOGET_FSCREENINFO, do_fbioget_fscreeninfo_ioctl),
	IOCTL32_HANDLER(FBIOGETCMAP, do_fbiocmap_ioctl),
	IOCTL32_HANDLER(FBIOPUTCMAP, do_fbiocmap_ioctl),
	IOCTL32_DEFAULT(FBIOPAN_DISPLAY),
#endif /* CONFIG_FB */

	/* Big K */
	IOCTL32_DEFAULT(PIO_FONT),
	IOCTL32_DEFAULT(GIO_FONT),
	IOCTL32_DEFAULT(KDSIGACCEPT),
	IOCTL32_DEFAULT(KDGETKEYCODE),
	IOCTL32_DEFAULT(KDSETKEYCODE),
	IOCTL32_DEFAULT(KIOCSOUND),
	IOCTL32_DEFAULT(KDMKTONE),
	IOCTL32_DEFAULT(KDGKBTYPE),
	IOCTL32_DEFAULT(KDSETMODE),
	IOCTL32_DEFAULT(KDGETMODE),
	IOCTL32_DEFAULT(KDSKBMODE),
	IOCTL32_DEFAULT(KDGKBMODE),
	IOCTL32_DEFAULT(KDSKBMETA),
	IOCTL32_DEFAULT(KDGKBMETA),
	IOCTL32_DEFAULT(KDGKBENT),
	IOCTL32_DEFAULT(KDSKBENT),
	IOCTL32_DEFAULT(KDGKBSENT),
	IOCTL32_DEFAULT(KDSKBSENT),
	IOCTL32_DEFAULT(KDGKBDIACR),
	IOCTL32_DEFAULT(KDSKBDIACR),
	IOCTL32_DEFAULT(KDKBDREP),
	IOCTL32_DEFAULT(KDGKBLED),
	IOCTL32_DEFAULT(KDSKBLED),
	IOCTL32_DEFAULT(KDGETLED),
	IOCTL32_DEFAULT(KDSETLED),
	IOCTL32_DEFAULT(GIO_SCRNMAP),
	IOCTL32_DEFAULT(PIO_SCRNMAP),
	IOCTL32_DEFAULT(GIO_UNISCRNMAP),
	IOCTL32_DEFAULT(PIO_UNISCRNMAP),
	IOCTL32_DEFAULT(PIO_FONTRESET),
	IOCTL32_DEFAULT(PIO_UNIMAPCLR),

	/* Big S */
	IOCTL32_DEFAULT(SCSI_IOCTL_GET_IDLUN),
	IOCTL32_DEFAULT(SCSI_IOCTL_DOORLOCK),
	IOCTL32_DEFAULT(SCSI_IOCTL_DOORUNLOCK),
	IOCTL32_DEFAULT(SCSI_IOCTL_TEST_UNIT_READY),
	IOCTL32_DEFAULT(SCSI_IOCTL_TAGGED_ENABLE),
	IOCTL32_DEFAULT(SCSI_IOCTL_TAGGED_DISABLE),
	IOCTL32_DEFAULT(SCSI_IOCTL_GET_BUS_NUMBER),
	IOCTL32_DEFAULT(SCSI_IOCTL_SEND_COMMAND),

	/* Big V */
	IOCTL32_DEFAULT(VT_SETMODE),
	IOCTL32_DEFAULT(VT_GETMODE),
	IOCTL32_DEFAULT(VT_GETSTATE),
	IOCTL32_DEFAULT(VT_OPENQRY),
	IOCTL32_DEFAULT(VT_ACTIVATE),
	IOCTL32_DEFAULT(VT_WAITACTIVE),
	IOCTL32_DEFAULT(VT_RELDISP),
	IOCTL32_DEFAULT(VT_DISALLOCATE),
	IOCTL32_DEFAULT(VT_RESIZE),
	IOCTL32_DEFAULT(VT_RESIZEX),
	IOCTL32_DEFAULT(VT_LOCKSWITCH),
	IOCTL32_DEFAULT(VT_UNLOCKSWITCH),

#ifdef CONFIG_NET
	/* Socket level stuff */
	IOCTL32_DEFAULT(FIOSETOWN),
	IOCTL32_DEFAULT(SIOCSPGRP),
	IOCTL32_DEFAULT(FIOGETOWN),
	IOCTL32_DEFAULT(SIOCGPGRP),
	IOCTL32_DEFAULT(SIOCATMARK),
	IOCTL32_DEFAULT(SIOCSIFLINK),
	IOCTL32_DEFAULT(SIOCSIFENCAP),
	IOCTL32_DEFAULT(SIOCGIFENCAP),
	IOCTL32_DEFAULT(SIOCSIFBR),
	IOCTL32_DEFAULT(SIOCGIFBR),
	IOCTL32_DEFAULT(SIOCSARP),
	IOCTL32_DEFAULT(SIOCGARP),
	IOCTL32_DEFAULT(SIOCDARP),
	IOCTL32_DEFAULT(SIOCSRARP),
	IOCTL32_DEFAULT(SIOCGRARP),
	IOCTL32_DEFAULT(SIOCDRARP),
	IOCTL32_DEFAULT(SIOCADDDLCI),
	IOCTL32_DEFAULT(SIOCDELDLCI),
	/* SG stuff */
	IOCTL32_DEFAULT(SG_SET_TIMEOUT),
	IOCTL32_DEFAULT(SG_GET_TIMEOUT),
	IOCTL32_DEFAULT(SG_EMULATED_HOST),
	IOCTL32_DEFAULT(SG_SET_TRANSFORM),
	IOCTL32_DEFAULT(SG_GET_TRANSFORM),
	IOCTL32_DEFAULT(SG_SET_RESERVED_SIZE),
	IOCTL32_DEFAULT(SG_GET_RESERVED_SIZE),
	IOCTL32_DEFAULT(SG_GET_SCSI_ID),
	IOCTL32_DEFAULT(SG_SET_FORCE_LOW_DMA),
	IOCTL32_DEFAULT(SG_GET_LOW_DMA),
	IOCTL32_DEFAULT(SG_SET_FORCE_PACK_ID),
	IOCTL32_DEFAULT(SG_GET_PACK_ID),
	IOCTL32_DEFAULT(SG_GET_NUM_WAITING),
	IOCTL32_DEFAULT(SG_SET_DEBUG),
	IOCTL32_DEFAULT(SG_GET_SG_TABLESIZE),
	IOCTL32_DEFAULT(SG_GET_COMMAND_Q),
	IOCTL32_DEFAULT(SG_SET_COMMAND_Q),
	IOCTL32_DEFAULT(SG_GET_VERSION_NUM),
	IOCTL32_DEFAULT(SG_NEXT_CMD_LEN),
	IOCTL32_DEFAULT(SG_SCSI_RESET),
	IOCTL32_DEFAULT(SG_IO),
	IOCTL32_DEFAULT(SG_GET_REQUEST_TABLE),
	IOCTL32_DEFAULT(SG_SET_KEEP_ORPHAN),
	IOCTL32_DEFAULT(SG_GET_KEEP_ORPHAN),
	/* PPP stuff */
	IOCTL32_DEFAULT(PPPIOCGFLAGS),
	IOCTL32_DEFAULT(PPPIOCSFLAGS),
	IOCTL32_DEFAULT(PPPIOCGASYNCMAP),
	IOCTL32_DEFAULT(PPPIOCSASYNCMAP),
	IOCTL32_DEFAULT(PPPIOCGUNIT),
	IOCTL32_DEFAULT(PPPIOCGRASYNCMAP),
	IOCTL32_DEFAULT(PPPIOCSRASYNCMAP),
	IOCTL32_DEFAULT(PPPIOCGMRU),
	IOCTL32_DEFAULT(PPPIOCSMRU),
	IOCTL32_DEFAULT(PPPIOCSMAXCID),
	IOCTL32_DEFAULT(PPPIOCGXASYNCMAP),
	IOCTL32_DEFAULT(PPPIOCSXASYNCMAP),
	IOCTL32_DEFAULT(PPPIOCXFERUNIT),
	IOCTL32_DEFAULT(PPPIOCGNPMODE),
	IOCTL32_DEFAULT(PPPIOCSNPMODE),
	IOCTL32_DEFAULT(PPPIOCGDEBUG),
	IOCTL32_DEFAULT(PPPIOCSDEBUG),
	IOCTL32_DEFAULT(PPPIOCNEWUNIT),
	IOCTL32_DEFAULT(PPPIOCATTACH),
	IOCTL32_DEFAULT(PPPIOCGCHAN),
	/* PPPOX */
	IOCTL32_DEFAULT(PPPOEIOCSFWD),
	IOCTL32_DEFAULT(PPPOEIOCDFWD),
	/* CDROM stuff */
	IOCTL32_DEFAULT(CDROMPAUSE),
	IOCTL32_DEFAULT(CDROMRESUME),
	IOCTL32_DEFAULT(CDROMPLAYMSF),
	IOCTL32_DEFAULT(CDROMPLAYTRKIND),
	IOCTL32_DEFAULT(CDROMREADTOCHDR),
	IOCTL32_DEFAULT(CDROMREADTOCENTRY),
	IOCTL32_DEFAULT(CDROMSTOP),
	IOCTL32_DEFAULT(CDROMSTART),
	IOCTL32_DEFAULT(CDROMEJECT),
	IOCTL32_DEFAULT(CDROMVOLCTRL),
	IOCTL32_DEFAULT(CDROMSUBCHNL),
	IOCTL32_DEFAULT(CDROMEJECT_SW),
	IOCTL32_DEFAULT(CDROMMULTISESSION),
	IOCTL32_DEFAULT(CDROM_GET_MCN),
	IOCTL32_DEFAULT(CDROMRESET),
	IOCTL32_DEFAULT(CDROMVOLREAD),
	IOCTL32_DEFAULT(CDROMSEEK),
	IOCTL32_DEFAULT(CDROMPLAYBLK),
	IOCTL32_DEFAULT(CDROMCLOSETRAY),
	IOCTL32_DEFAULT(CDROM_SET_OPTIONS),
	IOCTL32_DEFAULT(CDROM_CLEAR_OPTIONS),
	IOCTL32_DEFAULT(CDROM_SELECT_SPEED),
	IOCTL32_DEFAULT(CDROM_SELECT_DISC),
	IOCTL32_DEFAULT(CDROM_MEDIA_CHANGED),
	IOCTL32_DEFAULT(CDROM_DRIVE_STATUS),
	IOCTL32_DEFAULT(CDROM_DISC_STATUS),
	IOCTL32_DEFAULT(CDROM_CHANGER_NSLOTS),
	IOCTL32_DEFAULT(CDROM_LOCKDOOR),
	IOCTL32_DEFAULT(CDROM_DEBUG),
	IOCTL32_DEFAULT(CDROM_GET_CAPABILITY),
	/* DVD ioctls */
	IOCTL32_DEFAULT(DVD_READ_STRUCT),
	IOCTL32_DEFAULT(DVD_WRITE_STRUCT),
	IOCTL32_DEFAULT(DVD_AUTH),
	/* Big L */
	IOCTL32_DEFAULT(LOOP_SET_FD),
	IOCTL32_DEFAULT(LOOP_CLR_FD),

	/* And these ioctls need translation */
	IOCTL32_HANDLER(SIOCGIFNAME, dev_ifname32),
	IOCTL32_HANDLER(SIOCGIFCONF, dev_ifconf),
	IOCTL32_HANDLER(SIOCGIFFLAGS, dev_ifsioc),
	IOCTL32_HANDLER(SIOCSIFFLAGS, dev_ifsioc),
	IOCTL32_HANDLER(SIOCGIFMETRIC, dev_ifsioc),
	IOCTL32_HANDLER(SIOCSIFMETRIC, dev_ifsioc),
	IOCTL32_HANDLER(SIOCGIFMTU, dev_ifsioc),
	IOCTL32_HANDLER(SIOCSIFMTU, dev_ifsioc),
	IOCTL32_HANDLER(SIOCGIFMEM, dev_ifsioc),
	IOCTL32_HANDLER(SIOCSIFMEM, dev_ifsioc),
	IOCTL32_HANDLER(SIOCGIFHWADDR, dev_ifsioc),
	IOCTL32_HANDLER(SIOCSIFHWADDR, dev_ifsioc),
	IOCTL32_HANDLER(SIOCADDMULTI, dev_ifsioc),
	IOCTL32_HANDLER(SIOCDELMULTI, dev_ifsioc),
	IOCTL32_HANDLER(SIOCGIFINDEX, dev_ifsioc),
	IOCTL32_HANDLER(SIOCGIFMAP, dev_ifsioc),
	IOCTL32_HANDLER(SIOCSIFMAP, dev_ifsioc),
	IOCTL32_HANDLER(SIOCGIFADDR, dev_ifsioc),
	IOCTL32_HANDLER(SIOCSIFADDR, dev_ifsioc),
	IOCTL32_HANDLER(SIOCGIFBRDADDR, dev_ifsioc),
	IOCTL32_HANDLER(SIOCSIFBRDADDR, dev_ifsioc),
	IOCTL32_HANDLER(SIOCGIFDSTADDR, dev_ifsioc),
	IOCTL32_HANDLER(SIOCSIFDSTADDR, dev_ifsioc),
	IOCTL32_HANDLER(SIOCGIFNETMASK, dev_ifsioc),
	IOCTL32_HANDLER(SIOCSIFNETMASK, dev_ifsioc),
	IOCTL32_HANDLER(SIOCSIFPFLAGS, dev_ifsioc),
	IOCTL32_HANDLER(SIOCGIFPFLAGS, dev_ifsioc),
	IOCTL32_HANDLER(SIOCGPPPSTATS, dev_ifsioc),
	IOCTL32_HANDLER(SIOCGPPPCSTATS, dev_ifsioc),
	IOCTL32_HANDLER(SIOCGPPPVER, dev_ifsioc),
	IOCTL32_HANDLER(SIOCGIFTXQLEN, dev_ifsioc),
	IOCTL32_HANDLER(SIOCSIFTXQLEN, dev_ifsioc),
	IOCTL32_HANDLER(SIOCADDRT, routing_ioctl),
	IOCTL32_HANDLER(SIOCDELRT, routing_ioctl),
	/*
	 * Note SIOCRTMSG is no longer, so this is safe and * the user would
	 * have seen just an -EINVAL anyways.
	 */
	IOCTL32_HANDLER(SIOCRTMSG, ret_einval),
	IOCTL32_HANDLER(SIOCGSTAMP, do_siocgstamp),

#endif /* CONFIG_NET */

	IOCTL32_DEFAULT(PROF_START),
	IOCTL32_DEFAULT(PROF_STOP),
	IOCTL32_DEFAULT(PROF_RESET),
	IOCTL32_DEFAULT(PROF_SET_SAMPLE_FREQ),
	IOCTL32_DEFAULT(PROF_GET_SAMPLE_FREQ),
	IOCTL32_DEFAULT(PROF_GET_PC_RES),
	IOCTL32_DEFAULT(PROF_GET_ON_OFF_STATE),
	IOCTL32_DEFAULT(PROF_SET_DOMAIN),
	IOCTL32_DEFAULT(PROF_GET_DOMAIN),
	IOCTL32_DEFAULT(PROF_SET_MODE),
	IOCTL32_DEFAULT(PROF_GET_MODE),
	IOCTL32_DEFAULT(PROF_SET_PERFCTR_EVENT),
	IOCTL32_DEFAULT(PROF_GET_PERFCTR_EVENT),
	IOCTL32_DEFAULT(PROF_SET_ENABLE_MAP),
	IOCTL32_DEFAULT(PROF_GET_ENABLE_MAP),
	IOCTL32_DEFAULT(PROF_SET_PID),
	IOCTL32_DEFAULT(PROF_GET_PID),

	IOCTL32_HANDLER(EXT2_IOC32_GETFLAGS, do_ext2_ioctl),
	IOCTL32_HANDLER(EXT2_IOC32_SETFLAGS, do_ext2_ioctl),
	IOCTL32_HANDLER(EXT2_IOC32_GETVERSION, do_ext2_ioctl),
	IOCTL32_HANDLER(EXT2_IOC32_SETVERSION, do_ext2_ioctl),

	IOCTL32_HANDLER(HDIO_GETGEO, hdio_getgeo),	/* hdreg.h ioctls  */
	IOCTL32_HANDLER(HDIO_GET_UNMASKINTR, hdio_ioctl_trans),
	IOCTL32_HANDLER(HDIO_GET_MULTCOUNT, hdio_ioctl_trans),
	// HDIO_OBSOLETE_IDENTITY
	IOCTL32_HANDLER(HDIO_GET_KEEPSETTINGS, hdio_ioctl_trans),
	IOCTL32_HANDLER(HDIO_GET_32BIT, hdio_ioctl_trans),
	IOCTL32_HANDLER(HDIO_GET_NOWERR, hdio_ioctl_trans),
	IOCTL32_HANDLER(HDIO_GET_DMA, hdio_ioctl_trans),
	IOCTL32_HANDLER(HDIO_GET_NICE, hdio_ioctl_trans),
	IOCTL32_DEFAULT(HDIO_GET_IDENTITY),
	IOCTL32_DEFAULT(HDIO_DRIVE_RESET),
	// HDIO_TRISTATE_HWIF				/* not implemented */
	// HDIO_DRIVE_TASK				/* To do, need specs */
	IOCTL32_DEFAULT(HDIO_DRIVE_CMD),
	IOCTL32_DEFAULT(HDIO_SET_MULTCOUNT),
	IOCTL32_DEFAULT(HDIO_SET_UNMASKINTR),
	IOCTL32_DEFAULT(HDIO_SET_KEEPSETTINGS),
	IOCTL32_DEFAULT(HDIO_SET_32BIT),
	IOCTL32_DEFAULT(HDIO_SET_NOWERR),
	IOCTL32_DEFAULT(HDIO_SET_DMA),
	IOCTL32_DEFAULT(HDIO_SET_PIO_MODE),
	IOCTL32_DEFAULT(HDIO_SCAN_HWIF),
	IOCTL32_DEFAULT(HDIO_SET_NICE),
	//HDIO_UNREGISTER_HWIF

	IOCTL32_DEFAULT(BLKROSET),			/* fs.h ioctls  */
	IOCTL32_DEFAULT(BLKROGET),
	IOCTL32_DEFAULT(BLKRRPART),
	IOCTL32_HANDLER(BLKGETSIZE, w_long),

	IOCTL32_DEFAULT(BLKFLSBUF),
	IOCTL32_DEFAULT(BLKRASET),
	IOCTL32_HANDLER(BLKRAGET, w_long),
	IOCTL32_DEFAULT(BLKFRASET),
	IOCTL32_HANDLER(BLKFRAGET, w_long),
	IOCTL32_DEFAULT(BLKSECTSET),
	IOCTL32_HANDLER(BLKSECTGET, w_long),
	IOCTL32_DEFAULT(BLKSSZGET),
	IOCTL32_HANDLER(BLKPG, blkpg_ioctl_trans),
	IOCTL32_DEFAULT(BLKELVGET),
	IOCTL32_DEFAULT(BLKELVSET),
	IOCTL32_DEFAULT(BLKBSZGET),
	IOCTL32_DEFAULT(BLKBSZSET),

#ifdef CONFIG_MD
	/* status */
	IOCTL32_DEFAULT(RAID_VERSION),
	IOCTL32_DEFAULT(GET_ARRAY_INFO),
	IOCTL32_DEFAULT(GET_DISK_INFO),
	IOCTL32_DEFAULT(PRINT_RAID_DEBUG),
	IOCTL32_DEFAULT(RAID_AUTORUN),

	/* configuration */
	IOCTL32_DEFAULT(CLEAR_ARRAY),
	IOCTL32_DEFAULT(ADD_NEW_DISK),
	IOCTL32_DEFAULT(HOT_REMOVE_DISK),
	IOCTL32_DEFAULT(SET_ARRAY_INFO),
	IOCTL32_DEFAULT(SET_DISK_INFO),
	IOCTL32_DEFAULT(WRITE_RAID_INFO),
	IOCTL32_DEFAULT(UNPROTECT_ARRAY),
	IOCTL32_DEFAULT(PROTECT_ARRAY),
	IOCTL32_DEFAULT(HOT_ADD_DISK),
	IOCTL32_DEFAULT(SET_DISK_FAULTY),

	/* usage */
	IOCTL32_DEFAULT(RUN_ARRAY),
	IOCTL32_DEFAULT(START_ARRAY),
	IOCTL32_DEFAULT(STOP_ARRAY),
	IOCTL32_DEFAULT(STOP_ARRAY_RO),
	IOCTL32_DEFAULT(RESTART_ARRAY_RW),
#endif /* CONFIG_MD */

	IOCTL32_DEFAULT(MTIOCTOP),			/* mtio.h ioctls  */
	IOCTL32_HANDLER(MTIOCGET32, mt_ioctl_trans),
	IOCTL32_HANDLER(MTIOCPOS32, mt_ioctl_trans),
	IOCTL32_HANDLER(MTIOCGETCONFIG32, mt_ioctl_trans),
	IOCTL32_HANDLER(MTIOCSETCONFIG32, mt_ioctl_trans),
	// MTIOCRDFTSEG
	// MTIOCWRFTSEG
	// MTIOCVOLINFO
	// MTIOCGETSIZE
	// MTIOCFTFORMAT
	// MTIOCFTCMD

	IOCTL32_DEFAULT(AUTOFS_IOC_READY),		/* auto_fs.h ioctls */
	IOCTL32_DEFAULT(AUTOFS_IOC_FAIL),
	IOCTL32_DEFAULT(AUTOFS_IOC_CATATONIC),
	IOCTL32_DEFAULT(AUTOFS_IOC_PROTOVER),
	IOCTL32_HANDLER(AUTOFS_IOC_SETTIMEOUT32, ioc_settimeout),
	IOCTL32_DEFAULT(AUTOFS_IOC_EXPIRE),
	IOCTL32_DEFAULT(AUTOFS_IOC_EXPIRE_MULTI),

	/* Little p (/dev/rtc, /dev/envctrl, etc.) */
	IOCTL32_DEFAULT(_IOR('p', 20, int[7])), /* RTCGET */
	IOCTL32_DEFAULT(_IOW('p', 21, int[7])), /* RTCSET */
	IOCTL32_DEFAULT(RTC_AIE_ON),
	IOCTL32_DEFAULT(RTC_AIE_OFF),
	IOCTL32_DEFAULT(RTC_UIE_ON),
	IOCTL32_DEFAULT(RTC_UIE_OFF),
	IOCTL32_DEFAULT(RTC_PIE_ON),
	IOCTL32_DEFAULT(RTC_PIE_OFF),
	IOCTL32_DEFAULT(RTC_WIE_ON),
	IOCTL32_DEFAULT(RTC_WIE_OFF),
	IOCTL32_DEFAULT(RTC_ALM_SET),
	IOCTL32_DEFAULT(RTC_ALM_READ),
	IOCTL32_DEFAULT(RTC_RD_TIME),
	IOCTL32_DEFAULT(RTC_SET_TIME),
	IOCTL32_DEFAULT(RTC_WKALM_SET),
	IOCTL32_DEFAULT(RTC_WKALM_RD)
};

#define NR_IOCTL32_HANDLERS	(sizeof(ioctl32_handler_table) /	\
				 sizeof(ioctl32_handler_table[0]))

static struct ioctl32_list *ioctl32_hash_table[1024];

static inline int ioctl32_hash(unsigned int cmd)
{
	return ((cmd >> 6) ^ (cmd >> 4) ^ cmd) & 0x3ff;
}

int sys32_ioctl(unsigned int fd, unsigned int cmd, unsigned int arg)
{
	int (*handler)(unsigned int, unsigned int, unsigned long, struct file * filp);
	struct file *filp;
	struct ioctl32_list *l;
	int error;

	l = ioctl32_hash_table[ioctl32_hash(cmd)];

	error = -EBADF;

	filp = fget(fd);
	if (!filp)
		return error;

	if (!filp->f_op || !filp->f_op->ioctl) {
		error = sys_ioctl (fd, cmd, arg);
		goto out;
	}

	while (l && l->handler.cmd != cmd)
		l = l->next;

	if (l) {
		handler = (void *)l->handler.function;
		error = handler(fd, cmd, arg, filp);
	} else {
		error = -EINVAL;
		printk("unknown ioctl: %08x\n", cmd);
	}
out:
	fput(filp);
	return error;
}

static void ioctl32_insert(struct ioctl32_list *entry)
{
	int hash = ioctl32_hash(entry->handler.cmd);
	if (!ioctl32_hash_table[hash])
		ioctl32_hash_table[hash] = entry;
	else {
		struct ioctl32_list *l;
		l = ioctl32_hash_table[hash];
		while (l->next)
			l = l->next;
		l->next = entry;
		entry->next = 0;
	}
}

static int __init init_ioctl32(void)
{
	int i;
	for (i = 0; i < NR_IOCTL32_HANDLERS; i++)
		ioctl32_insert(&ioctl32_handler_table[i]);
	return 0;
}

__initcall(init_ioctl32);
