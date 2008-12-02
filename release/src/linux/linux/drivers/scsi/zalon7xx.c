/*
 * Zalon 53c7xx device driver.
 * By Richard Hirst (rhirst@linuxcare.com)
 */

#include <linux/init.h>
#include <linux/types.h>
#include <linux/stat.h>
#include <linux/mm.h>
#include <linux/blk.h>
#include <linux/sched.h>
#include <linux/version.h>
#include <linux/config.h>
#include <linux/module.h>

#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/irq.h>
#include <asm/hardware.h>
#include <asm/delay.h>
#include <asm/gsc.h>

#include "scsi.h"
#include "hosts.h"

/*
 * **      Define the BSD style u_int32 and u_int64 type.
 * **      Are in fact u_int32_t and u_int64_t :-)
 * */
typedef u32 u_int32;
typedef u64 u_int64;
typedef u_long          vm_offset_t;

#include "zalon7xx.h"


/* hosts_* are kluges to pass info between the zalon7xx_detected()
** and the register_parisc_driver() callbacks.
*/
static Scsi_Host_Template *hosts_tptr;
static int hosts_used=0;
static int zalon_id = 0;

extern int zalon_attach(Scsi_Host_Template *tpnt,
			unsigned long base_addr,
			struct parisc_device *dev,
			int irq_vector,
			int unit
			);



static int __init
zalon_scsi_callback(struct parisc_device *dev)
{
	struct gsc_irq gsc_irq;
	u32 zalon_vers;
	int irq;
	unsigned long zalon = dev->hpa;

	__raw_writel(CMD_RESET, zalon + IO_MODULE_IO_COMMAND);
	while (!(__raw_readl(zalon + IO_MODULE_IO_STATUS) & IOSTATUS_RY))
		;
	__raw_writel(IOIIDATA_MINT5EN | IOIIDATA_PACKEN | IOIIDATA_PREFETCHEN,
		zalon + IO_MODULE_II_CDATA);

	zalon_vers = __raw_readl(dev->hpa + IO_MODULE_II_CDATA) & 0x07000000;
	zalon_vers >>= 24;

	/* Setup the interrupts first.
	** Later on request_irq() will register the handler.
	*/
        irq = gsc_alloc_irq(&gsc_irq);

	printk("%s: Zalon vers field is 0x%x, IRQ %d\n", __FUNCTION__,
		zalon_vers, irq);

	__raw_writel(gsc_irq.txn_addr | gsc_irq.txn_data, dev->hpa + IO_MODULE_EIM);

	if ( zalon_vers == 0)
		printk(KERN_WARNING "%s: Zalon 1.1 or earlier\n", __FUNCTION__);

	/*
	**  zalon_attach: returns -1 on failure, 0 on success
	*/
	hosts_used = zalon_attach(hosts_tptr, dev->hpa + GSC_SCSI_ZALON_OFFSET,
			dev, irq, zalon_id);

	if (hosts_used == 0)
		zalon_id++;

	hosts_used = (hosts_used == 0);
	return (hosts_used == 0);
}

static struct parisc_device_id zalon_tbl[] = {
	{ HPHW_A_DMA, HVERSION_REV_ANY_ID, HVERSION_ANY_ID, 0x00089 }, 
	{ 0, }
};

MODULE_DEVICE_TABLE(parisc, zalon_tbl);

static struct parisc_driver zalon_driver = {
	name:		"GSC SCSI (Zalon)",
	id_table:	zalon_tbl,
	probe:		zalon_scsi_callback,
};

int zalon7xx_detect(Scsi_Host_Template *tpnt)
{
	/* "pass" the parameter to the callback functions */
	hosts_tptr = tpnt;
	hosts_used = 0;

	/* claim all zalon cards. */
	register_parisc_driver(&zalon_driver);

	/* Check if any callbacks actually found/claimed anything. */
	return (hosts_used != 0);
}

#ifdef MODULE
extern int ncr53c8xx_release(struct Scsi_Host *host);

int zalon7xx_release(struct Scsi_Host *host)
{
	ncr53c8xx_release(host);
	unregister_parisc_driver(&zalon_driver);
	return 1;
}
#endif
