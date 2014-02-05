#ifndef _TC_CORE_H_
#define _TC_CORE_H_ 1

#include <asm/types.h>
#include <linux/pkt_sched.h>

long tc_core_usec2tick(long usec);
long tc_core_tick2usec(long tick);
unsigned tc_calc_xmittime(unsigned rate, unsigned size);

void tc_calc_ratespec(struct tc_ratespec* spec, __u32* rtab, unsigned bps,
	int cell_log, unsigned mtu, unsigned char mpu, int atm_cell_tax,
	char overhead);

int tc_setup_estimator(unsigned A, unsigned time_const, struct tc_estimator *est);

int tc_core_init(void);

extern struct rtnl_handle g_rth;
extern int is_batch_mode;

#endif
