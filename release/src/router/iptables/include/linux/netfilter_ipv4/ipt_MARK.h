#ifndef _IPT_MARK_H_target
#define _IPT_MARK_H_target

struct ipt_mark_target_info {
#ifdef KERNEL_64_USERSPACE_32
	unsigned long long mark;
#else
	unsigned long mark;
#endif
};

enum {
	IPT_MARK_SET=0,
	IPT_MARK_AND,
    IPT_MARK_OR,
    IPT_MARK_SET_RETURN,
    IPT_MARK_AND_RETURN,
    IPT_MARK_OR_RETURN
};

struct ipt_mark_target_info_v1 {
#ifdef KERNEL_64_USERSPACE_32
	unsigned long long mark;
#else
	unsigned long mark;
#endif
	u_int8_t mode;
};

#endif /*_IPT_MARK_H_target*/
