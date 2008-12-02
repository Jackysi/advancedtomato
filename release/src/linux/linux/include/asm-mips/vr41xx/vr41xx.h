/*
 * include/asm-mips/vr41xx/vr41xx.h
 *
 * Include file for NEC VR4100 series.
 *
 * Copyright (C) 1999 Michael Klar
 * Copyright (C) 2001, 2002 Paul Mundt
 * Copyright (C) 2002 MontaVista Software, Inc.
 * Copyright (C) 2002 TimeSys Corp.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */
#ifndef __NEC_VR41XX_H
#define __NEC_VR41XX_H

#include <linux/interrupt.h>

/*
 * CPU Revision
 */
/* VR4122 0x00000c70-0x00000c72 */
#define PRID_VR4122_REV1_0	0x00000c70
#define PRID_VR4122_REV2_0	0x00000c70
#define PRID_VR4122_REV2_1	0x00000c70
#define PRID_VR4122_REV3_0	0x00000c71
#define PRID_VR4122_REV3_1	0x00000c72

/* VR4181A 0x00000c73-0x00000c7f */
#define PRID_VR4181A_REV1_0	0x00000c73
#define PRID_VR4181A_REV1_1	0x00000c74

/* VR4131 0x00000c80-0x00000c8f */
#define PRID_VR4131_REV1_2	0x00000c80
#define PRID_VR4131_REV2_0	0x00000c81
#define PRID_VR4131_REV2_1	0x00000c82
#define PRID_VR4131_REV2_2	0x00000c83

/*
 * Bus Control Uint
 */
extern void vr41xx_bcu_init(void);

/*
 * Clock Mask Unit
 */
extern void vr41xx_cmu_init(u16 mask);
extern void vr41xx_clock_supply(u16 mask);
extern void vr41xx_clock_mask(u16 mask);

/*
 * Interrupt Control Unit
 */

/* GIU Interrupt Numbers */
#define GIU_IRQ(x)	(40 + (x))

extern void (*board_irq_init)(void);
extern int vr41xx_cascade_irq(unsigned int irq, int (*get_irq_number)(int irq));

/*
 * Gegeral-Purpose I/O Unit
 */
extern void vr41xx_enable_giuint(int pin);
extern void vr41xx_disable_giuint(int pin);
extern void vr41xx_clear_giuint(int pin);

enum {
	TRIGGER_LEVEL,
	TRIGGER_EDGE
};

enum {
	SIGNAL_THROUGH,
	SIGNAL_HOLD
};

extern void vr41xx_set_irq_trigger(int pin, int trigger, int hold);

enum {
	LEVEL_LOW,
	LEVEL_HIGH
};

extern void vr41xx_set_irq_level(int pin, int level);

enum {
	PIO_INPUT,
	PIO_OUTPUT
};

enum {
	DATA_LOW,
	DATA_HIGH
};

/*
 * Serial Interface Unit
 */
extern void vr41xx_siu_init(int interface, int module);
extern void vr41xx_siu_ifselect(int interface, int module);
extern int vr41xx_serial_ports;

/* SIU interfaces */
enum {
	SIU_RS232C,
	SIU_IRDA
};

/* IrDA interfaces */
enum {
	IRDA_SHARP = 1,
	IRDA_TEMIC,
	IRDA_HP
};

/*
 * Debug Serial Interface Unit
 */
extern void vr41xx_dsiu_init(void);

/*
 * PCI Control Unit
 */
struct vr41xx_pci_address_space {
	u32 internal_base;
	u32 address_mask;
	u32 pci_base;
};

struct vr41xx_pci_address_map {
	struct vr41xx_pci_address_space *mem1;
	struct vr41xx_pci_address_space *mem2;
	struct vr41xx_pci_address_space *io;
};

extern void vr41xx_pciu_init(struct vr41xx_pci_address_map *map);

/*
 * MISC
 */
extern void vr41xx_time_init(void);
extern void vr41xx_timer_setup(struct irqaction *irq);

extern void vr41xx_restart(char *command);
extern void vr41xx_halt(void);
extern void vr41xx_power_off(void);

#endif /* __NEC_VR41XX_H */
