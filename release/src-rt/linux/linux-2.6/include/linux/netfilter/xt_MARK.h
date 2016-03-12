#ifndef _XT_MARK_H_target
#define _XT_MARK_H_target

/* Version 0 */
struct xt_mark_target_info {
	unsigned long mark;
};

/* Version 1 */
enum {
	XT_MARK_SET=0,
	XT_MARK_AND,
	XT_MARK_OR,
	XT_MARK_SET_RETURN,
	XT_MARK_AND_RETURN,
	XT_MARK_OR_RETURN,
};

struct xt_mark_target_info_v1 {
	unsigned long mark;
	u_int8_t mode;
};

#endif /*_XT_MARK_H_target */
