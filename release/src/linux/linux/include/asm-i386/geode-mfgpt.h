/*     Driver/API for AMD Geode Multi-Function General Purpose Timers (MFGPT)
 *
 *     Copyright (C) 2006, Advanced Micro Devices, Inc.
 *
 *      This program is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU General Public License
 *      as published by the Free Software Foundation; either version
 *      2 of the License, or (at your option) any later version.
 *
 */

#ifndef MFGPT_GEODE_H_
#define MFGPT_GEODE_H_

#define MFGPT_TIMER_ANY -1

#define MFGPT_DOMAIN_WORKING 1
#define MFGPT_DOMAIN_STANDBY 2
#define MFGPT_DOMAIN_ANY (MFGPT_DOMAIN_WORKING | MFGPT_DOMAIN_STANDBY)

#define MSR_MFGPT_IRQ		0x51400028
#define MSR_MFGPT_NR		0x51400029
#define MSR_MFGPT_SETUP		0x5140002B

#define MFGPT_MAX_TIMERS 8
#define MFGPT_PCI_BAR 2

#define MFGPT_CMP1 0
#define MFGPT_CMP2 1

#define MFGPT_EVENT_IRQ   0
#define MFGPT_EVENT_NMI   1
#define MFGPT_EVENT_RESET 3

#define MFGPT_REG_CMP1    0
#define MFGPT_REG_CMP2    2
#define MFGPT_REG_COUNTER 4
#define MFGPT_REG_SETUP   6

#define MFGPT_SETUP_CNTEN  (1 << 15)
#define MFGPT_SETUP_CMP2   (1 << 14)
#define MFGPT_SETUP_CMP1   (1 << 13)
#define MFGPT_SETUP_SETUP  (1 << 12)
#define MFGPT_SETUP_STOPEN (1 << 11)
#define MFGPT_SETUP_EXTEN  (1 << 10)
#define MFGPT_SETUP_REVEN  (1 << 5)
#define MFGPT_SETUP_CLKSEL (1 << 4)

extern int geode_mfgpt_toggle_event(int, int, int, int);

#define geode_mfgpt_set_event(t,c,e) geode_mfgpt_toggle_event(t,c,e,1)
#define geode_mfgpt_clear_event(t,c,e) geode_mfgpt_toggle_event(t,c,e,0)

extern void geode_mfgpt_set_irq(int, int, int, int);

#define geode_mfgpt_setup_irq(t, c, i) geode_mfgpt_set_irq(t,c,i,1)
#define geode_mfgpt_release_irq(t, c, i) geode_mfgpt_set_irq(t,c,i,0)

extern void geode_mfgpt_write(int, u16, u16);
extern u16 geode_mfgpt_read(int, u16);

extern int geode_mfgpt_alloc_timer(int, int);

/* Specific geode tests */

static inline int is_geode_gx(void)
{
        return ((boot_cpu_data.x86_vendor == X86_VENDOR_NSC) &&
                (boot_cpu_data.x86 == 5) &&
                (boot_cpu_data.x86_model == 5));
}

static inline int is_geode_lx(void)
{
        return ((boot_cpu_data.x86_vendor == X86_VENDOR_AMD) &&
                (boot_cpu_data.x86 == 5) &&
                (boot_cpu_data.x86_model == 10));
}

static inline int is_geode(void)
{
        return (is_geode_gx() || is_geode_lx());
}

#endif
