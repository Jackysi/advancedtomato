/*
 * Copyright (C) 2001 Anton Blanchard <anton@au.ibm.com>, IBM
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * Communication to userspace based on kernel/printk.c
 */

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/poll.h>
#include <linux/proc_fs.h>
#include <linux/init.h>
#include <linux/vmalloc.h>

#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/rtas.h>
#include <asm/prom.h>

#define DEBUG(A...)

static spinlock_t rtas_log_lock = SPIN_LOCK_UNLOCKED;

DECLARE_WAIT_QUEUE_HEAD(rtas_log_wait);

#define LOG_NUMBER		64		/* must be a power of two */
#define LOG_NUMBER_MASK		(LOG_NUMBER-1)

static char *rtas_log_buf;
static unsigned long rtas_log_start;
static unsigned long rtas_log_size;

static int surveillance_requested;
static unsigned int rtas_event_scan_rate;
static unsigned int rtas_error_log_max;

#define EVENT_SCAN_ALL_EVENTS	0xf0000000
#define SURVEILLANCE_TOKEN	9000
#define SURVEILLANCE_TIMEOUT	1
#define SURVEILLANCE_SCANRATE	1

/*
 * Since we use 32 bit RTAS, the physical address of this must be below
 * 4G or else bad things happen. Allocate this in the kernel data and
 * make it big enough.
 */
#define RTAS_ERROR_LOG_MAX 1024
static unsigned char logdata[RTAS_ERROR_LOG_MAX];

static int rtas_log_open(struct inode * inode, struct file * file)
{
	return 0;
}

static int rtas_log_release(struct inode * inode, struct file * file)
{
	return 0;
}

static ssize_t rtas_log_read(struct file * file, char * buf,
			 size_t count, loff_t *ppos)
{
	int error;
	char *tmp;
	unsigned long offset;

	if (!buf || count < rtas_error_log_max)
		return -EINVAL;

	count = rtas_error_log_max;

	error = verify_area(VERIFY_WRITE, buf, count);
	if (error)
		return -EINVAL;

	tmp = kmalloc(rtas_error_log_max, GFP_KERNEL);
	if (!tmp)
		return -ENOMEM;

	error = wait_event_interruptible(rtas_log_wait, rtas_log_size);
	if (error)
		goto out;

	spin_lock(&rtas_log_lock);
	offset = rtas_error_log_max * (rtas_log_start & LOG_NUMBER_MASK);
	memcpy(tmp, &rtas_log_buf[offset], count);
	rtas_log_start += 1;
	rtas_log_size -= 1;
	spin_unlock(&rtas_log_lock);

	error = copy_to_user(buf, tmp, count) ? -EFAULT : count;
out:
	kfree(tmp);
	return error;
}

static unsigned int rtas_log_poll(struct file *file, poll_table * wait)
{
	poll_wait(file, &rtas_log_wait, wait);
	if (rtas_log_size)
		return POLLIN | POLLRDNORM;
	return 0;
}

struct file_operations proc_rtas_log_operations = {
	read:		rtas_log_read,
	poll:		rtas_log_poll,
	open:		rtas_log_open,
	release:	rtas_log_release,
};


#define RTAS_ERR KERN_ERR "RTAS: "

/* Extended error log header (12 bytes) */
struct exthdr {
	unsigned int valid:1;
	unsigned int unrecoverable:1;
	unsigned int recoverable:1;
	unsigned int unrecoverable_bypassed:1;	/* i.e. degraded performance */
	unsigned int predictive:1;
	unsigned int newlog:1;
	unsigned int bigendian:1;		/* always 1 */
	unsigned int /* reserved */:1;

	unsigned int platform_specific:1;	/* only in version 3+ */
	unsigned int /* reserved */:3;
	unsigned int platform_value:4;		/* valid iff platform_specific */

	unsigned int power_pc:1;		/* always 1 */
	unsigned int /* reserved */:2;
	unsigned int addr_invalid:1;		/* failing_address is invalid */
	unsigned int format_type:4;
#define EXTLOG_FMT_CPU 1
#define EXTLOG_FMT_MEMORY 2
#define EXTLOG_FMT_IO 3
#define EXTLOG_FMT_POST 4
#define EXTLOG_FMT_ENV 5
#define EXTLOG_FMT_POW 6
#define EXTLOG_FMT_IBMDIAG 12
#define EXTLOG_FMT_IBMSP 13

	/* This group is in version 3+ only */
	unsigned int non_hardware:1;		/* Firmware or software is suspect */
	unsigned int hot_plug:1;		/* Failing component may be hot plugged */
	unsigned int group_failure:1;		/* Group of components should be replaced */
	unsigned int /* reserved */:1;

	unsigned int residual:1;		/* Residual error from previous boot (maybe a crash) */
	unsigned int boot:1;			/* Error during boot */
	unsigned int config_change:1;		/* Configuration changed since last boot */
	unsigned int post:1;			/* Error during POST */

	unsigned int bcdtime:32;		/* Time of error in BCD HHMMSS00 */
	unsigned int bcddate:32;		/* Time of error in BCD YYYYMMDD */
};

struct cpuhdr {
	unsigned int internal:1;
	unsigned int intcache:1;
	unsigned int extcache_parity:1;	/* or multi-bit ECC */
	unsigned int extcache_ecc:1;
	unsigned int sysbus_timeout:1;
	unsigned int io_timeout:1;
	unsigned int sysbus_parity:1;
	unsigned int sysbus_protocol:1;
	unsigned int cpuid:8;
	unsigned int element:16;
	unsigned int failing_address_hi:32;
	unsigned int failing_address_lo:32;

	/* These are version 4+ */
	unsigned int try_reboot:1;	/* 1 => fault may be fixed by reboot */
	unsigned int /* reserved */:7;
	/* 15 bytes reserved here */
};

struct memhdr {
	unsigned int uncorrectable:1;
	unsigned int ECC:1;
	unsigned int threshold_exceeded:1;
	unsigned int control_internal:1;
	unsigned int bad_address:1;
	unsigned int bad_data:1;
	unsigned int bus:1;
	unsigned int timeout:1;
	unsigned int sysbus_parity:1;
	unsigned int sysbus_timeout:1;
	unsigned int sysbus_protocol:1;
	unsigned int hostbridge_timeout:1;
	unsigned int hostbridge_parity:1;
	unsigned int reserved1:1;
	unsigned int support:1;
	unsigned int sysbus_internal:1;
	unsigned int mem_controller_detected:8;	/* who detected fault? */
	unsigned int mem_controller_faulted:8;	/* who caused fault? */
	unsigned int failing_address_hi:32;
	unsigned int failing_address_lo:32;
	unsigned int ecc_syndrome:16;
	unsigned int memory_card:8;
	unsigned int reserved2:8;
	unsigned int sub_elements:32;		/* one bit per element */
	unsigned int element:16;
};

struct iohdr {
	unsigned int bus_addr_parity:1;
	unsigned int bus_data_parity:1;
	unsigned int bus_timeout:1;
	unsigned int bridge_internal:1;
	unsigned int non_pci:1;		/* i.e. secondary bus such as ISA */
	unsigned int mezzanine_addr_parity:1;
	unsigned int mezzanine_data_parity:1;
	unsigned int mezzanine_timeout:1;

	unsigned int bridge_via_sysbus:1;
	unsigned int bridge_via_mezzanine:1;
	unsigned int bridge_via_expbus:1;
	unsigned int detected_by_expbus:1;
	unsigned int expbus_data_parity:1;
	unsigned int expbus_timeout:1;
	unsigned int expbus_connection_failure:1;
	unsigned int expbus_not_operating:1;

	/* IOA signalling the error */
	unsigned int pci_sig_busno:8;
	unsigned int pci_sig_devfn:8;
	unsigned int pci_sig_deviceid:16;
	unsigned int pci_sig_vendorid:16;
	unsigned int pci_sig_revisionid:8;
	unsigned int pci_sig_slot:8;	/* 00 => system board, ff => multiple */

	/* IOA sending at time of error */
	unsigned int pci_send_busno:8;
	unsigned int pci_send_devfn:8;
	unsigned int pci_send_deviceid:16;
	unsigned int pci_send_vendorid:16;
	unsigned int pci_send_revisionid:8;
	unsigned int pci_send_slot:8;	/* 00 => system board, ff => multiple */
};

struct posthdr {
	unsigned int firmware:1;
	unsigned int config:1;
	unsigned int cpu:1;
	unsigned int memory:1;
	unsigned int io:1;
	unsigned int keyboard:1;
	unsigned int mouse:1;
	unsigned int display:1;

	unsigned int ipl_floppy:1;
	unsigned int ipl_controller:1;
	unsigned int ipl_cdrom:1;
	unsigned int ipl_disk:1;
	unsigned int ipl_net:1;
	unsigned int ipl_other:1;
	unsigned int /* reserved */:1;
	unsigned int firmware_selftest:1;

	char         devname[12];
	unsigned int post_code:4;
	unsigned int firmware_rev:2;
	unsigned int loc_code:8;	/* currently unused */
};

struct epowhdr {
	unsigned int epow_sensor_value:32;
	unsigned int sensor:1;
	unsigned int power_fault:1;
	unsigned int fan:1;
	unsigned int temp:1;
	unsigned int redundancy:1;
	unsigned int CUoD:1;
	unsigned int /* reserved */:2;

	unsigned int general:1;
	unsigned int power_loss:1;
	unsigned int power_supply:1;
	unsigned int power_switch:1;
	unsigned int /* reserved */:4;

	unsigned int /* reserved */:16;
	unsigned int sensor_token:32;
	unsigned int sensor_index:32;
	unsigned int sensor_value:32;
	unsigned int sensor_status:32;
};

struct pm_eventhdr {
	unsigned int event_id:32;
};

struct sphdr {
	unsigned int ibm:32;	/* "IBM\0" */

	unsigned int timeout:1;
	unsigned int i2c_bus:1;
	unsigned int i2c_secondary_bus:1;
	unsigned int sp_memory:1;
	unsigned int sp_registers:1;
	unsigned int sp_communication:1;
	unsigned int sp_firmware:1;
	unsigned int sp_hardware:1;

	unsigned int vpd_eeprom:1;
	unsigned int op_panel:1;
	unsigned int power_controller:1;
	unsigned int fan_sensor:1;
	unsigned int thermal_sensor:1;
	unsigned int voltage_sensor:1;
	unsigned int reserved1:2;

	unsigned int serial_port:1;
	unsigned int nvram:1;
	unsigned int rtc:1;
	unsigned int jtag:1;
	unsigned int tod_battery:1;
	unsigned int reserved2:1;
	unsigned int heartbeat:1;
	unsigned int surveillance:1;

	unsigned int pcn_connection:1;	/* power control network */
	unsigned int pcn_node:1;
	unsigned int reserved3:2;
	unsigned int pcn_access:1;
	unsigned int reserved:3;

	unsigned int sensor_token:32;	/* zero if undef */
	unsigned int sensor_index:32;	/* zero if undef */
};


static char *severity_names[] = {
	"NO ERROR", "EVENT", "WARNING", "ERROR_SYNC", "ERROR", "FATAL", "(6)", "(7)"
};
static char *rtas_disposition_names[] = {
	"FULLY RECOVERED", "LIMITED RECOVERY", "NOT RECOVERED", "(4)"
};
static char *entity_names[] = { /* for initiator & targets */
	"UNKNOWN", "CPU", "PCI", "ISA", "MEMORY", "POWER MANAGEMENT", "HOT PLUG", "(7)", "(8)",
	"(9)", "(10)", "(11)", "(12)", "(13)", "(14)", "(15)"
};
static char *error_type[] = {	/* Not all types covered here so need to bounds check */
	"UNKNOWN", "RETRY", "TCE_ERR", "INTERN_DEV_FAIL",
	"TIMEOUT", "DATA_PARITY", "ADDR_PARITY", "CACHE_PARITY",
	"ADDR_INVALID", "ECC_UNCORR", "ECC_CORR",
};

static char *rtas_error_type(int type)
{
	if (type < 11)
		return error_type[type];
	if (type == 64)
		return "SENSOR";
	if (type >=96 && type <= 159)
		return "POWER";
	return error_type[0];
}

static void printk_cpu_failure(int version, struct exthdr *exthdr, char *data)
{
	struct cpuhdr cpuhdr;

	memcpy(&cpuhdr, data, sizeof(cpuhdr));

	if (cpuhdr.internal) printk(RTAS_ERR "Internal error (not cache)\n");
	if (cpuhdr.intcache) printk(RTAS_ERR "Internal cache\n");
	if (cpuhdr.extcache_parity) printk(RTAS_ERR "External cache parity (or multi-bit)\n");
	if (cpuhdr.extcache_ecc) printk(RTAS_ERR "External cache ECC\n");
	if (cpuhdr.sysbus_timeout) printk(RTAS_ERR "System bus timeout\n");
	if (cpuhdr.io_timeout) printk(RTAS_ERR "I/O timeout\n");
	if (cpuhdr.sysbus_parity) printk(RTAS_ERR "System bus parity\n");
	if (cpuhdr.sysbus_protocol) printk(RTAS_ERR "System bus protocol/transfer\n");
	printk(RTAS_ERR "CPU id: %d\n", cpuhdr.cpuid);
	printk(RTAS_ERR "Failing element: 0x%04x\n", cpuhdr.element);
	if (!exthdr->addr_invalid)
		printk(RTAS_ERR "Failing address: %08x%08x\n", cpuhdr.failing_address_hi, cpuhdr.failing_address_lo);
	if (version >= 4 && cpuhdr.try_reboot)
		printk(RTAS_ERR "A reboot of the system may correct the problem\n");
}

static void printk_mem_failure(int version, struct exthdr *exthdr, char *data)
{
	struct memhdr memhdr;

	memcpy(&memhdr, data, sizeof(memhdr));
	if (memhdr.uncorrectable) printk(RTAS_ERR "Uncorrectable Memory error\n");
	if (memhdr.ECC) printk(RTAS_ERR "ECC Correctable error\n");
	if (memhdr.threshold_exceeded) printk(RTAS_ERR "Correctable threshold exceeded\n");
	if (memhdr.control_internal) printk(RTAS_ERR "Memory Controller internal error\n");
	if (memhdr.bad_address) printk(RTAS_ERR "Memory Address error\n");
	if (memhdr.bad_data) printk(RTAS_ERR "Memory Data error\n");
	if (memhdr.bus) printk(RTAS_ERR "Memory bus/switch internal error\n");
	if (memhdr.timeout) printk(RTAS_ERR "Memory timeout\n");
	if (memhdr.sysbus_parity) printk(RTAS_ERR "System bus parity\n");
	if (memhdr.sysbus_timeout) printk(RTAS_ERR "System bus timeout\n");
	if (memhdr.sysbus_protocol) printk(RTAS_ERR "System bus protocol/transfer\n");
	if (memhdr.hostbridge_timeout) printk(RTAS_ERR "I/O Host Bridge timeout\n");
	if (memhdr.hostbridge_parity) printk(RTAS_ERR "I/O Host Bridge parity\n");
	if (memhdr.support) printk(RTAS_ERR "System support function error\n");
	if (memhdr.sysbus_internal) printk(RTAS_ERR "System bus internal hardware/switch error\n");
	printk(RTAS_ERR "Memory Controller that detected failure: %d\n", memhdr.mem_controller_detected);
	printk(RTAS_ERR "Memory Controller that faulted: %d\n", memhdr.mem_controller_faulted);
	if (!exthdr->addr_invalid)
		printk(RTAS_ERR "Failing address: 0x%016x%016x\n", memhdr.failing_address_hi, memhdr.failing_address_lo);
	printk(RTAS_ERR "ECC syndrome bits: 0x%04x\n", memhdr.ecc_syndrome);
	printk(RTAS_ERR "Memory Card: %d\n", memhdr.memory_card);
	printk(RTAS_ERR "Failing element: 0x%04x\n", memhdr.element);
	printk(RTAS_ERR "Sub element bits: 0x%08x\n", memhdr.sub_elements);
}

static void printk_io_failure(int version, struct exthdr *exthdr, char *data)
{
	struct iohdr iohdr;

	memcpy(&iohdr, data, sizeof(iohdr));
	if (iohdr.bus_addr_parity) printk(RTAS_ERR "I/O bus address parity\n");
	if (iohdr.bus_data_parity) printk(RTAS_ERR "I/O bus data parity\n");
	if (iohdr.bus_timeout) printk(RTAS_ERR "I/O bus timeout, access or other\n");
	if (iohdr.bridge_internal) printk(RTAS_ERR "I/O bus bridge/device internal\n");
	if (iohdr.non_pci) printk(RTAS_ERR "Signaling IOA is a PCI to non-PCI bridge (e.g. ISA)\n");
	if (iohdr.mezzanine_addr_parity) printk(RTAS_ERR "Mezzanine/System bus address parity\n");
	if (iohdr.mezzanine_data_parity) printk(RTAS_ERR "Mezzanine/System bus data parity\n");
	if (iohdr.mezzanine_timeout) printk(RTAS_ERR "Mezzanine/System bus timeout, transfer or protocol\n");
	if (iohdr.bridge_via_sysbus) printk(RTAS_ERR "Bridge is connected to system bus\n");
	if (iohdr.bridge_via_mezzanine) printk(RTAS_ERR "Bridge is connected to memory controller via mezzanine bus\n");
	if (iohdr.bridge_via_expbus) printk(RTAS_ERR "Bridge is connected to I/O expansion bus\n");
	if (iohdr.detected_by_expbus) printk(RTAS_ERR "Error on system bus detected by I/O expansion bus controller\n");
	if (iohdr.expbus_data_parity) printk(RTAS_ERR "I/O expansion bus data error\n");
	if (iohdr.expbus_timeout) printk(RTAS_ERR "I/O expansion bus timeout, access or other\n");
	if (iohdr.expbus_connection_failure) printk(RTAS_ERR "I/O expansion bus connection failure\n");
	if (iohdr.expbus_not_operating) printk(RTAS_ERR "I/O expansion unit not in an operating state (powered off, off-line)\n");

	printk(RTAS_ERR "IOA Signaling the error: %d:%d.%d vendor:%04x device:%04x rev:%02x slot:%d\n",
	       iohdr.pci_sig_busno, iohdr.pci_sig_devfn >> 3, iohdr.pci_sig_devfn & 0x7,
	       iohdr.pci_sig_vendorid, iohdr.pci_sig_deviceid, iohdr.pci_sig_revisionid, iohdr.pci_sig_slot);
	printk(RTAS_ERR "IOA Sending during the error: %d:%d.%d vendor:%04x device:%04x rev:%02x slot:%d\n",
	       iohdr.pci_send_busno, iohdr.pci_send_devfn >> 3, iohdr.pci_send_devfn & 0x7,
	       iohdr.pci_send_vendorid, iohdr.pci_send_deviceid, iohdr.pci_send_revisionid, iohdr.pci_send_slot);

}

static void printk_post_failure(int version, struct exthdr *exthdr, char *data)
{
	struct posthdr posthdr;

	memcpy(&posthdr, data, sizeof(posthdr));

	if (posthdr.devname[0]) printk(RTAS_ERR "Failing Device: %s\n", posthdr.devname);
	if (posthdr.firmware) printk(RTAS_ERR "Firmware Error\n");
	if (posthdr.config) printk(RTAS_ERR "Configuration Error\n");
	if (posthdr.cpu) printk(RTAS_ERR "CPU POST Error\n");
	if (posthdr.memory) printk(RTAS_ERR "Memory POST Error\n");
	if (posthdr.io) printk(RTAS_ERR "I/O Subsystem POST Error\n");
	if (posthdr.keyboard) printk(RTAS_ERR "Keyboard POST Error\n");
	if (posthdr.mouse) printk(RTAS_ERR "Mouse POST Error\n");
	if (posthdr.display) printk(RTAS_ERR "Display POST Error\n");

	if (posthdr.ipl_floppy) printk(RTAS_ERR "Floppy IPL Error\n");
	if (posthdr.ipl_controller) printk(RTAS_ERR "Drive Controller Error during IPL\n");
	if (posthdr.ipl_cdrom) printk(RTAS_ERR "CDROM IPL Error\n");
	if (posthdr.ipl_disk) printk(RTAS_ERR "Disk IPL Error\n");
	if (posthdr.ipl_net) printk(RTAS_ERR "Network IPL Error\n");
	if (posthdr.ipl_other) printk(RTAS_ERR "Other (tape,flash) IPL Error\n");
	if (posthdr.firmware_selftest) printk(RTAS_ERR "Self-test error in firmware extended diagnostics\n");
	printk(RTAS_ERR "POST Code: %d\n", posthdr.post_code);
	printk(RTAS_ERR "Firmware Revision Code: %d\n", posthdr.firmware_rev);
}

static void printk_epow_warning(int version, struct exthdr *exthdr, char *data)
{
	struct epowhdr epowhdr;

	memcpy(&epowhdr, data, sizeof(epowhdr));
	printk(RTAS_ERR "EPOW Sensor Value:  0x%08x\n", epowhdr.epow_sensor_value); 
	if (epowhdr.sensor) {
		printk(RTAS_ERR "EPOW detected by a sensor\n");
		printk(RTAS_ERR "Sensor Token:  0x%08x\n", epowhdr.sensor_token); 
		printk(RTAS_ERR "Sensor Index:  0x%08x\n", epowhdr.sensor_index); 
		printk(RTAS_ERR "Sensor Value:  0x%08x\n", epowhdr.sensor_value); 
		printk(RTAS_ERR "Sensor Status: 0x%08x\n", epowhdr.sensor_status);
	}
	if (epowhdr.power_fault) printk(RTAS_ERR "EPOW caused by a power fault\n");
	if (epowhdr.fan) printk(RTAS_ERR "EPOW caused by fan failure\n");
	if (epowhdr.temp) printk(RTAS_ERR "EPOW caused by over-temperature condition\n");
	if (epowhdr.redundancy) printk(RTAS_ERR "EPOW warning due to loss of redundancy\n");
	if (epowhdr.CUoD) printk(RTAS_ERR "EPOW warning due to CUoD Entitlement Exceeded\n");

	if (epowhdr.general) printk(RTAS_ERR "EPOW general power fault\n");
	if (epowhdr.power_loss) printk(RTAS_ERR "EPOW power fault due to loss of power source\n");
	if (epowhdr.power_supply) printk(RTAS_ERR "EPOW power fault due to internal power supply failure\n");
	if (epowhdr.power_switch) printk(RTAS_ERR "EPOW power fault due to activation of power switch\n");
}

static void printk_pm_event(int version, struct exthdr *exthdr, char *data)
{
	struct pm_eventhdr pm_eventhdr;

	memcpy(&pm_eventhdr, data, sizeof(pm_eventhdr));
	printk(RTAS_ERR "Event id: 0x%08x\n", pm_eventhdr.event_id);
}

static void printk_sp_log_msg(int version, struct exthdr *exthdr, char *data)
{
	struct sphdr sphdr;
	u32 eyecatcher;

	memcpy(&sphdr, data, sizeof(sphdr));

	eyecatcher = sphdr.ibm;
	if (strcmp((char *)&eyecatcher, "IBM") != 0)
		printk(RTAS_ERR "This log entry may be corrupt (IBM signature malformed)\n");
	if (sphdr.timeout) printk(RTAS_ERR "Timeout on communication response from service processor\n");
	if (sphdr.i2c_bus) printk(RTAS_ERR "I2C general bus error\n");
	if (sphdr.i2c_secondary_bus) printk(RTAS_ERR "I2C secondary bus error\n");
	if (sphdr.sp_memory) printk(RTAS_ERR "Internal service processor memory error\n");
	if (sphdr.sp_registers) printk(RTAS_ERR "Service processor error accessing special registers\n");
	if (sphdr.sp_communication) printk(RTAS_ERR "Service processor reports unknown communcation error\n");
	if (sphdr.sp_firmware) printk(RTAS_ERR "Internal service processor firmware error\n");
	if (sphdr.sp_hardware) printk(RTAS_ERR "Other internal service processor hardware error\n");
	if (sphdr.vpd_eeprom) printk(RTAS_ERR "Service processor error accessing VPD EEPROM\n");
	if (sphdr.op_panel) printk(RTAS_ERR "Service processor error accessing Operator Panel\n");
	if (sphdr.power_controller) printk(RTAS_ERR "Service processor error accessing Power Controller\n");
	if (sphdr.fan_sensor) printk(RTAS_ERR "Service processor error accessing Fan Sensor\n");
	if (sphdr.thermal_sensor) printk(RTAS_ERR "Service processor error accessing Thermal Sensor\n");
	if (sphdr.voltage_sensor) printk(RTAS_ERR "Service processor error accessing Voltage Sensor\n");
	if (sphdr.serial_port) printk(RTAS_ERR "Service processor error accessing serial port\n");
	if (sphdr.nvram) printk(RTAS_ERR "Service processor detected NVRAM error\n");
	if (sphdr.rtc) printk(RTAS_ERR "Service processor error accessing real time clock\n");
	if (sphdr.jtag) printk(RTAS_ERR "Service processor error accessing JTAG/COP\n");
	if (sphdr.tod_battery) printk(RTAS_ERR "Service processor or RTAS detects loss of voltage from TOD battery\n");
	if (sphdr.heartbeat) printk(RTAS_ERR "Loss of heartbeat from Service processor\n");
	if (sphdr.surveillance) printk(RTAS_ERR "Service processor detected a surveillance timeout\n");
	if (sphdr.pcn_connection) printk(RTAS_ERR "Power Control Network general connection failure\n");
	if (sphdr.pcn_node) printk(RTAS_ERR "Power Control Network node failure\n");
	if (sphdr.pcn_access) printk(RTAS_ERR "Service processor error accessing Power Control Network\n");

	if (sphdr.sensor_token) printk(RTAS_ERR "Sensor Token 0x%08x (%d)\n", sphdr.sensor_token, sphdr.sensor_token);
	if (sphdr.sensor_index) printk(RTAS_ERR "Sensor Index 0x%08x (%d)\n", sphdr.sensor_index, sphdr.sensor_index);
}


static void printk_ext_raw_data(char *data)
{
	int i;
	printk(RTAS_ERR "raw ext data: ");
	for (i = 0; i < 40; i++) {
		printk("%02x", data[i]);
	}
	printk("\n");
}

static void printk_ext_log_data(int version, char *buf)
{
	char *data = buf+12;
	struct exthdr exthdr;
	memcpy(&exthdr, buf, sizeof(exthdr));	/* copy for alignment */
	if (!exthdr.valid) {
		if (exthdr.bigendian && exthdr.power_pc)
			printk(RTAS_ERR "extended log data is not valid\n");
		else
			printk(RTAS_ERR "extended log data can not be decoded\n");
		return;
	}

	/* Dump useful stuff in the exthdr */
	printk(RTAS_ERR "Status:%s%s%s%s%s\n",
	       exthdr.unrecoverable ? " unrecoverable" : "",
	       exthdr.recoverable ? " recoverable" : "",
	       exthdr.unrecoverable_bypassed ? " bypassed" : "",
	       exthdr.predictive ? " predictive" : "",
	       exthdr.newlog ? " new" : "");
	printk(RTAS_ERR "Date/Time: %08x %08x\n", exthdr.bcddate, exthdr.bcdtime);
	switch (exthdr.format_type) {
	    case EXTLOG_FMT_CPU:
		printk(RTAS_ERR "CPU Failure\n");
		printk_cpu_failure(version, &exthdr, data);
		break;
	    case EXTLOG_FMT_MEMORY:
		printk(RTAS_ERR "Memory Failure\n");
		printk_mem_failure(version, &exthdr, data);
		break;
	    case EXTLOG_FMT_IO:
		printk(RTAS_ERR "I/O Failure\n");
		printk_io_failure(version, &exthdr, data);
		break;
	    case EXTLOG_FMT_POST:
		printk(RTAS_ERR "POST Failure\n");
		printk_post_failure(version, &exthdr, data);
		break;
	    case EXTLOG_FMT_ENV:
		printk(RTAS_ERR "Environment and Power Warning\n");
		printk_epow_warning(version, &exthdr, data);
		break;
	    case EXTLOG_FMT_POW:
		printk(RTAS_ERR "Power Management Event\n");
		printk_pm_event(version, &exthdr, data);
		break;
	    case EXTLOG_FMT_IBMDIAG:
		printk(RTAS_ERR "IBM Diagnostic Log\n");
		printk_ext_raw_data(data);
		break;
	    case EXTLOG_FMT_IBMSP:
		printk(RTAS_ERR "IBM Service Processor Log\n");
		printk_sp_log_msg(version, &exthdr, data);
		break;
	    default:
		printk(RTAS_ERR "Unknown ext format type %d\n", exthdr.format_type);
		printk_ext_raw_data(data);
		break;
	}
}

#define MAX_LOG_DEBUG 10
/* Print log debug data.  This appears after the location code.
 * We limit the number of debug logs in case the data is somehow corrupt.
 */
static void printk_log_debug(char *buf)
{
	unsigned char *p = (unsigned char *)_ALIGN((unsigned long)buf, 8);
	int len, n, logged;

	logged = 0;
	while ((logged < MAX_LOG_DEBUG) && (len = ((p[0] << 8) | p[1])) != 2) {
		/* len includes 2-byte length thus len == 2 is the end */
		printk("RTAS: Log Debug: ");
		if (len >= 4)	/* next 2 bytes are an ascii code */
			printk("%c%c ", p[2], p[3]);
		for (n=4; n < len; n++)
			printk("%02x", p[n]);
		printk("\n");
		p += len;
		logged++;
	}
	if (logged == 0)
		printk("RTAS: no log debug data present\n");
}


/* Yeah, the output here is ugly, but we want a CE to be
 * able to grep RTAS /var/log/messages and see all the info
 * collected together with obvious begin/end.
 */
static void printk_log_rtas(char *buf)
{
	struct rtas_error_log *err = (struct rtas_error_log *)buf;

	printk(RTAS_ERR "-------- event-scan begin --------\n");
	if (strcmp(buf+8+40, "IBM") == 0) {
		/* Location code follows */
		char *loc = buf+8+40+4;
		int len = strlen(loc);
		if (len < 64) {	/* Sanity check */
			printk(RTAS_ERR "Location Code: %s\n", loc);
			printk_log_debug(loc+len+1);
		}
	}

	printk(RTAS_ERR "%s: (%s) type: %s\n",
	       severity_names[err->severity],
	       rtas_disposition_names[err->disposition],
	       rtas_error_type(err->type));
	printk(RTAS_ERR "initiator: %s  target: %s\n",
	       entity_names[err->initiator], entity_names[err->target]);
	if (err->extended_log_length)
		printk_ext_log_data(err->version, buf+8);
	printk(RTAS_ERR "-------- event-scan end ----------\n");
}


static void log_rtas(char *buf)
{
	unsigned long offset;

	DEBUG("logging rtas event\n");

	/* Temporary -- perhaps we can do this when nobody has the log open? */
	printk_log_rtas(buf);

	spin_lock(&rtas_log_lock);

	offset = rtas_error_log_max *
			((rtas_log_start+rtas_log_size) & LOG_NUMBER_MASK);

	memcpy(&rtas_log_buf[offset], buf, rtas_error_log_max);

	if (rtas_log_size < LOG_NUMBER)
		rtas_log_size += 1;
	else
		rtas_log_start += 1;

	spin_unlock(&rtas_log_lock);
	wake_up_interruptible(&rtas_log_wait);
}

static int enable_surveillance(void)
{
	int error;

	error = rtas_call(rtas_token("set-indicator"), 3, 1, NULL, SURVEILLANCE_TOKEN,
			0, SURVEILLANCE_TIMEOUT);

	if (error) {
		printk(KERN_ERR "rtasd: could not enable surveillance\n");
		return -1;
	}

	rtas_event_scan_rate = SURVEILLANCE_SCANRATE;

	return 0;
}

static int get_eventscan_parms(void)
{
	struct device_node *node;
	int *ip;

	node = find_path_device("/rtas");

	ip = (int *)get_property(node, "rtas-event-scan-rate", NULL);
	if (ip == NULL) {
		printk(KERN_ERR "rtasd: no rtas-event-scan-rate\n");
		return -1;
	}
	rtas_event_scan_rate = *ip;
	DEBUG("rtas-event-scan-rate %d\n", rtas_event_scan_rate);

	ip = (int *)get_property(node, "rtas-error-log-max", NULL);
	if (ip == NULL) {
		printk(KERN_ERR "rtasd: no rtas-error-log-max\n");
		return -1;
	}
	rtas_error_log_max = *ip;
	DEBUG("rtas-error-log-max %d\n", rtas_error_log_max);

	if (rtas_error_log_max > RTAS_ERROR_LOG_MAX) {
		printk(KERN_ERR "rtasd: truncated error log from %d to %d bytes\n", rtas_error_log_max, RTAS_ERROR_LOG_MAX);
		rtas_error_log_max = RTAS_ERROR_LOG_MAX;
	}

	return 0;
}

extern long sys_sched_get_priority_max(int policy);

static int rtasd(void *unused)
{
	int cpu = 0;
	int error;
	int first_pass = 1;
	int event_scan = rtas_token("event-scan");

	if (event_scan == RTAS_UNKNOWN_SERVICE || get_eventscan_parms() == -1)
		goto error;

	rtas_log_buf = vmalloc(rtas_error_log_max*LOG_NUMBER);
	if (!rtas_log_buf) {
		printk(KERN_ERR "rtasd: no memory\n");
		goto error;
	}

	DEBUG("will sleep for %d jiffies\n", (HZ*60/rtas_event_scan_rate) / 2);

	daemonize();
	sigfillset(&current->blocked);
	sprintf(current->comm, "rtasd");

	/* Rusty unreal time task */
	current->policy = SCHED_FIFO;
	current->nice = sys_sched_get_priority_max(SCHED_FIFO) + 1;

	cpu = 0;
	current->cpus_allowed = 1UL << cpu_logical_map(cpu);
	schedule();

	while(1) {
		do {
			memset(logdata, 0, rtas_error_log_max);
			error = rtas_call(event_scan, 4, 1, NULL,
					EVENT_SCAN_ALL_EVENTS, 0,
					__pa(logdata), rtas_error_log_max);
			if (error == -1) {
				printk(KERN_ERR "event-scan failed\n");
				break;
			}

			if (error == 0)
				log_rtas(logdata);

		} while(error == 0);

		DEBUG("watchdog scheduled on cpu %d\n", smp_processor_id());

		cpu++;
		if (cpu >= smp_num_cpus) {

			if (first_pass && surveillance_requested) {
				DEBUG("enabling surveillance\n");
				if (enable_surveillance())
					goto error_vfree;
				DEBUG("surveillance enabled\n");
			}

			first_pass = 0;
			cpu = 0;
		}

		current->cpus_allowed = 1UL << cpu_logical_map(cpu);

		/* Check all cpus for pending events before sleeping*/
		if (first_pass) {
			schedule();
		} else {
			set_current_state(TASK_INTERRUPTIBLE);
			schedule_timeout((HZ*60/rtas_event_scan_rate) / 2);
		}
	}

error_vfree:
	vfree(rtas_log_buf);
error:
	/* Should delete proc entries */
	return -EINVAL;
}

static void __init rtas_init(void)
{
	struct proc_dir_entry *rtas_dir, *entry;

	rtas_dir = proc_mkdir("rtas", 0);
	if (!rtas_dir) {
		printk(KERN_ERR "Failed to create rtas proc directory\n");
	} else {
		entry = create_proc_entry("error_log", S_IRUSR, rtas_dir);
		if (entry)
			entry->proc_fops = &proc_rtas_log_operations;
		else
			printk(KERN_ERR "Failed to create rtas/error_log proc entry\n");
	}

	if (kernel_thread(rtasd, 0, CLONE_FS) < 0)
		printk(KERN_ERR "Failed to start RTAS daemon\n");

	printk(KERN_ERR "RTAS daemon started\n");
}

static int __init surveillance_setup(char *str)
{
	int i;

	if (get_option(&str,&i)) {
		if (i == 1)
			surveillance_requested = 1;
	}

	return 1;
}

__initcall(rtas_init);
__setup("surveillance=", surveillance_setup);
