


#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/pci.h>		/* pci cfg accessor functions  */
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/smp_lock.h>
#include <linux/interrupt.h>	/* irqaction */
#include <linux/irq.h>		/* irq_region support */

#include <asm/byteorder.h>	/* get in-line asm for swab */
#include <asm/pdc.h>
#include <asm/page.h>
#include <asm/segment.h>
#include <asm/system.h>
#include <asm/gsc.h>		/* gsc_read/write functions */
#ifdef CONFIG_SUPERIO
#include <asm/superio.h>
#endif

#include <asm/iosapic.h>
#include "./iosapic_private.h"

#define MODULE_NAME "iosapic"

/* "local" compile flags */
#undef PCI_BRIDGE_FUNCS
#undef DEBUG_IOSAPIC
#undef DEBUG_IOSAPIC_IRT


#ifdef DEBUG_IOSAPIC
static char assert_buf[128];

static int
assert_failed (char *a, char *f, int l)
{
        sprintf(assert_buf,
			"ASSERT(%s) failed!\nline %d in %s\n",
			a,      /* assertion text */
			l,      /* line number */
			f);     /* file name */
        panic(assert_buf);
	return 0;
}

#undef ASSERT
#define ASSERT(EX) { if (!(EX)) assert_failed(# EX, __FILE__, __LINE__); }

#define DBG(x...) printk(x)

#else /* DEBUG_IOSAPIC */

#define DBG(x...)
#define ASSERT(EX)

#endif /* DEBUG_IOSAPIC */

#ifdef DEBUG_IOSAPIC_IRT
#define DBG_IRT(x...) printk(x)
#else
#define DBG_IRT(x...)
#endif


#define READ_U8(addr)  gsc_readb(addr)
#define READ_U16(addr) le16_to_cpu(gsc_readw((u16 *) (addr)))
#define READ_U32(addr) le32_to_cpu(gsc_readl((u32 *) (addr)))
#define READ_REG16(addr) gsc_readw((u16 *) (addr))
#define READ_REG32(addr) gsc_readl((u32 *) (addr))
#define WRITE_U8(value, addr) gsc_writeb(value, addr)
#define WRITE_U16(value, addr) gsc_writew(cpu_to_le16(value), (u16 *) (addr))
#define WRITE_U32(value, addr) gsc_writel(cpu_to_le32(value), (u32 *) (addr))
#define WRITE_REG16(value, addr) gsc_writew(value, (u16 *) (addr))
#define WRITE_REG32(value, addr) gsc_writel(value, (u32 *) (addr))


#define IOSAPIC_REG_SELECT              0
#define IOSAPIC_REG_WINDOW              0x10
#define IOSAPIC_REG_EOI                 0x40

#define IOSAPIC_REG_VERSION		0x1

#define IOSAPIC_IRDT_ENTRY(idx)		(0x10+(idx)*2)
#define IOSAPIC_IRDT_ENTRY_HI(idx)	(0x11+(idx)*2)

#define	IOSAPIC_KALLOC(a_type, cnt) \
			(a_type *) kmalloc(sizeof(a_type)*(cnt), GFP_KERNEL)
#define IOSAPIC_FREE(addr, f_type, cnt) kfree((void *)addr)


#define	IOSAPIC_LOCK(lck)	spin_lock_irqsave(lck, irqflags)
#define	IOSAPIC_UNLOCK(lck)	spin_unlock_irqrestore(lck, irqflags)


#define IOSAPIC_VERSION_MASK            0x000000ff
#define IOSAPIC_VERSION_SHIFT           0x0
#define	IOSAPIC_VERSION(ver)				\
		(int) ((ver & IOSAPIC_VERSION_MASK) >> IOSAPIC_VERSION_SHIFT)

#define IOSAPIC_MAX_ENTRY_MASK          0x00ff0000

#define IOSAPIC_MAX_ENTRY_SHIFT         0x10
#define	IOSAPIC_IRDT_MAX_ENTRY(ver)			\
		(int) ((ver&IOSAPIC_MAX_ENTRY_MASK) >> IOSAPIC_MAX_ENTRY_SHIFT)

/* bits in the "low" I/O Sapic IRdT entry */
#define IOSAPIC_IRDT_ENABLE       0x10000
#define IOSAPIC_IRDT_PO_LOW       0x02000
#define IOSAPIC_IRDT_LEVEL_TRIG   0x08000
#define IOSAPIC_IRDT_MODE_LPRI    0x00100

/* bits in the "high" I/O Sapic IRdT entry */
#define IOSAPIC_IRDT_ID_EID_SHIFT              0x10



#define	IOSAPIC_EOI(eoi_addr, eoi_data) gsc_writel(eoi_data, eoi_addr)

static struct iosapic_info *iosapic_list;
static spinlock_t iosapic_lock;
static int iosapic_count;


/*
** REVISIT: future platforms may have more than one IRT.
** If so, the following three fields form a structure which
** then be linked into a list. Names are chosen to make searching
** for them easy - not necessarily accurate (eg "cell").
**
** Alternative: iosapic_info could point to the IRT it's in.
** iosapic_register() could search a list of IRT's.
*/
static struct irt_entry *irt_cell;
static size_t irt_num_entry;



/*
** iosapic_load_irt
**
** The "Get PCI INT Routing Table Size" option returns the number of 
** entries in the PCI interrupt routing table for the cell specified 
** in the cell_number argument.  The cell number must be for a cell 
** within the caller's protection domain.
**
** The "Get PCI INT Routing Table" option returns, for the cell 
** specified in the cell_number argument, the PCI interrupt routing 
** table in the caller allocated memory pointed to by mem_addr.
** We assume the IRT only contains entries for I/O SAPIC and
** calculate the size based on the size of I/O sapic entries.
**
** The PCI interrupt routing table entry format is derived from the
** IA64 SAL Specification 2.4.   The PCI interrupt routing table defines
** the routing of PCI interrupt signals between the PCI device output
** "pins" and the IO SAPICs' input "lines" (including core I/O PCI
** devices).  This table does NOT include information for devices/slots
** behind PCI to PCI bridges. See PCI to PCI Bridge Architecture Spec.
** for the architected method of routing of IRQ's behind PPB's.
*/


static int __init /* return number of entries as success/fail flag */
iosapic_load_irt(unsigned long cell_num, struct irt_entry **irt)
{
	long status;              /* PDC return value status */
	struct irt_entry *table;  /* start of interrupt routing tbl */
	unsigned long num_entries = 0UL;

	ASSERT(NULL != irt);

	if (is_pdc_pat()) {

		/* Use pat pdc routine to get interrupt routing table size */
		DBG("calling get_irt_size (cell %ld)\n", cell_num);
		status = pdc_pat_get_irt_size(&num_entries, cell_num);
		DBG("get_irt_size: %ld\n", status);

		ASSERT(status == PDC_OK);

		/* save the number of entries in the table */
		ASSERT(0UL != num_entries);

		/*
		** allocate memory for interrupt routing table
		** This interface isn't really right. We are assuming
		** the contents of the table are exclusively
		** for I/O sapic devices.
		*/
		table = IOSAPIC_KALLOC(struct irt_entry, num_entries);
		if (table == NULL) {
			printk(KERN_WARNING MODULE_NAME ": read_irt : can not alloc mem for IRT\n");
			return 0;
		}

		/* get PCI INT routing table */
		status = pdc_pat_get_irt(table, cell_num);
		DBG("pdc_pat_get_irt: %ld\n", status);
		ASSERT(status == PDC_OK);
	} else {
		/*
		** C3000/J5000 (and similar) platforms with Sprockets PDC
		** will return exactly one IRT for all iosapics.
		** So if we have one, don't need to get it again.
		*/
		if (NULL != irt_cell)
			return 0;

		/* Should be using the Elroy's HPA, but it's ignored anyway */
		status = pdc_pci_irt_size(&num_entries, 0);
		DBG("pdc_pci_irt_size: %ld\n", status);

		if (PDC_OK != status) {
			/* Not a "legacy" system with I/O SAPIC either */
			return 0;
		}

		ASSERT(0UL != num_entries);

		table = IOSAPIC_KALLOC(struct irt_entry, num_entries);
		if (table == NULL) {
			printk(KERN_WARNING MODULE_NAME ": read_irt : can not alloc mem for IRT\n");
			return 0;
		}

		/* HPA ignored by this call too. */
		status = pdc_pci_irt(num_entries, 0, table);
		ASSERT(PDC_OK == status);
	}

	/* return interrupt table address */
	*irt = table;

#ifdef DEBUG_IOSAPIC_IRT
{
	struct irt_entry *p = table;
	int i;

	printk(KERN_DEBUG MODULE_NAME " Interrupt Routing Table (cell %ld)\n", cell_num);
	printk(KERN_DEBUG MODULE_NAME " start = 0x%p num_entries %ld entry_size %d\n",
		table,
		num_entries,
		(int) sizeof(struct irt_entry));

	for (i = 0 ; i < num_entries ; i++, p++) {
		printk(KERN_DEBUG MODULE_NAME " %02x %02x %02x %02x %02x %02x %02x %02x %08x%08x\n",
		p->entry_type, p->entry_length, p->interrupt_type,
		p->polarity_trigger, p->src_bus_irq_devno, p->src_bus_id,
		p->src_seg_id, p->dest_iosapic_intin,
		((u32 *) p)[2],
		((u32 *) p)[3]
		);
	}
}
#endif /* DEBUG_IOSAPIC_IRT */

	return num_entries;
}



void __init
iosapic_init(void)
{
	unsigned long cell = 0;

	/* init global data */
	iosapic_lock = SPIN_LOCK_UNLOCKED;
        iosapic_list = (struct iosapic_info *) NULL;
	iosapic_count = 0;

	DBG("iosapic_init()\n");

#ifdef __LP64__
	if (is_pdc_pat()) {
		int status;
		struct pdc_pat_cell_num cell_info;

		status = pdc_pat_cell_get_number(&cell_info);
		if (status == PDC_OK) {
			cell = cell_info.cell_num;
		}
	}
#endif

	/*
	**  get IRT for this cell.
	*/
	irt_num_entry =  iosapic_load_irt(cell, &irt_cell);
	if (0 == irt_num_entry)
		irt_cell = NULL;	/* old PDC w/o iosapic */
}


/*
** Return the IRT entry in case we need to look something else up.
*/
static struct irt_entry *
irt_find_irqline(struct iosapic_info *isi, u8 slot, u8 intr_pin)
{
	struct irt_entry *i = irt_cell;
	int cnt;	/* track how many entries we've looked at */
	u8 irq_devno = (slot << IRT_DEV_SHIFT) | (intr_pin-1);

	DBG_IRT("irt_find_irqline() SLOT %d pin %d\n", slot, intr_pin);

	for (cnt=0; cnt < irt_num_entry; cnt++, i++) {

		/*
		** Validate: entry_type, entry_length, interrupt_type
		**
		** Difference between validate vs compare is the former
		** should print debug info and is not expected to "fail"
		** on current platforms.
		*/
		if (i->entry_type != IRT_IOSAPIC_TYPE) {
			DBG_IRT(KERN_WARNING MODULE_NAME ":find_irqline(0x%p): skipping entry %d type %d\n", i, cnt, i->entry_type);
			continue;
		}
		
		if (i->entry_length != IRT_IOSAPIC_LENGTH) {
			DBG_IRT(KERN_WARNING MODULE_NAME ":find_irqline(0x%p): skipping entry %d  length %d\n", i, cnt, i->entry_length);
			continue;
		}

		if (i->interrupt_type != IRT_VECTORED_INTR) {
			DBG_IRT(KERN_WARNING MODULE_NAME ":find_irqline(0x%p): skipping entry  %d interrupt_type %d\n", i, cnt, i->interrupt_type);
			continue;
		}

		/*
		** Compare: dest_iosapic_addr, src_bus_irq_devno
		*/
		if (i->dest_iosapic_addr != (u64) ((long) isi->isi_hpa))
			continue;

		if ((i->src_bus_irq_devno & IRT_IRQ_DEVNO_MASK) != irq_devno)
			continue;

		/*
		** Ignore: src_bus_id and rc_seg_id correlate with
		**         iosapic_info->isi_hpa on HP platforms.
		**         If needed, pass in "PFA" (aka config space addr)
		**         instead of slot.
		*/

		/* Found it! */
		return i;
	}

	printk(KERN_WARNING MODULE_NAME ": 0x%lx : no IRT entry for slot %d, pin %d\n",
			isi->isi_hpa, slot, intr_pin);
	return NULL;
}


/*
** xlate_pin() supports the skewing of IRQ lines done by subsidiary bridges.
** Legacy PDC already does this translation for us and stores it in INTR_LINE.
**
** PAT PDC needs to basically do what legacy PDC does:
** o read PIN
** o adjust PIN in case device is "behind" a PPB
**     (eg 4-port 100BT and SCSI/LAN "Combo Card")
** o convert slot/pin to I/O SAPIC input line.
**
** HP platforms only support:
** o one level of skewing for any number of PPBs
** o only support PCI-PCI Bridges.
*/
static struct irt_entry *
iosapic_xlate_pin(struct iosapic_info *isi, struct pci_dev *pcidev)
{
	u8 intr_pin, intr_slot;

	pci_read_config_byte(pcidev, PCI_INTERRUPT_PIN, &intr_pin);

	DBG_IRT("iosapic_xlate_pin() SLOT %d pin %d\n",
		PCI_SLOT(pcidev->devfn), intr_pin);

	if (0 == intr_pin) {
		/* The device does NOT support/use IRQ lines.  */
		return NULL;
	}

	/* Check if pcidev behind a PPB */
	if (NULL != pcidev->bus->self) {
		/* Convert pcidev INTR_PIN into something we
		** can lookup in the IRT.
		*/
#ifdef PCI_BRIDGE_FUNCS
		/*
		** Proposal #1:
		**
		** call implementation specific translation function
		** This is architecturally "cleaner". HP-UX doesn't
		** support other secondary bus types (eg. E/ISA) directly.
		** May be needed for other processor (eg IA64) architectures
		** or by some ambitous soul who wants to watch TV.
		*/
		if (pci_bridge_funcs->xlate_intr_line) {
			intr_pin = pci_bridge_funcs->xlate_intr_line(pcidev);
		}
#else	/* PCI_BRIDGE_FUNCS */
		struct pci_bus *p = pcidev->bus;
		/*
		** Proposal #2:
		** The "pin" is skewed ((pin + dev - 1) % 4).
		**
		** This isn't very clean since I/O SAPIC must assume:
		**   - all platforms only have PCI busses.
		**   - only PCI-PCI bridge (eg not PCI-EISA, PCI-PCMCIA)
		**   - IRQ routing is only skewed once regardless of
		**     the number of PPB's between iosapic and device.
		**     (Bit3 expansion chassis follows this rule)
		**
		** Advantage is it's really easy to implement.
		*/
		intr_pin = ((intr_pin-1)+PCI_SLOT(pcidev->devfn)) % 4;
		intr_pin++;	/* convert back to INTA-D (1-4) */
#endif /* PCI_BRIDGE_FUNCS */

		/*
		** Locate the host slot the PPB nearest the Host bus
		** adapter.
		*/
		while (NULL != p->parent->self)
			p = p->parent;

		intr_slot = PCI_SLOT(p->self->devfn);
	} else {
		intr_slot = PCI_SLOT(pcidev->devfn);
	}
	DBG_IRT("iosapic_xlate_pin:  bus %d slot %d pin %d\n",
				pcidev->bus->secondary, intr_slot, intr_pin);

	return irt_find_irqline(isi, intr_slot, intr_pin);
}


static void
iosapic_interrupt(int irq, void *dev_id, struct pt_regs * regs)
{
	struct vector_info *vi = (struct vector_info *)dev_id;
	extern void do_irq(struct irqaction *a, int i, struct pt_regs *p);
	int irq_num = vi->vi_ios->isi_region->data.irqbase + vi->vi_irqline;

	DBG("iosapic_interrupt(): irq %d line %d eoi %p\n",
		irq, vi->vi_irqline, vi->vi_eoi_addr);

	do_irq(&vi->vi_ios->isi_region->action[vi->vi_irqline], irq_num, regs);

	/*
	** PCI only supports level triggered in order to share IRQ lines.
	** I/O SAPIC must always issue EOI.
	*/
	IOSAPIC_EOI(vi->vi_eoi_addr, vi->vi_eoi_data);
}


int
iosapic_fixup_irq(void *isi_obj, struct pci_dev *pcidev)
{
	struct iosapic_info *isi = (struct iosapic_info *)isi_obj;
	struct irt_entry *irte = NULL;  /* only used if PAT PDC */
	struct vector_info *vi;
	int isi_line;	/* line used by device */
	int tmp;
	int return_irq;
#ifdef CONFIG_SUPERIO
	int superio_irq = -1;
#endif

	if (NULL == isi) {
		printk(KERN_WARNING MODULE_NAME ": hpa not registered for %s\n",
			pcidev->name);
		return(-1);
	}

#ifdef CONFIG_SUPERIO
	if (is_superio_device(pcidev)) {
		superio_irq = superio_fixup_irq(pcidev);
		if (superio_irq == -1)
		    return(-1);

		if (PCI_FUNC(pcidev->devfn) != SUPERIO_USB_FN) {

			/*
			 * SuperIO USB controller has an irt entry.
			 * Only let the USB controller hookup the rest
			 * of the interrupt routing when it comes through.
			 * Note that interrupts for all three functions
			 * actually come through the PIC's on function 1!
			 */

			pcidev->irq = superio_irq;
			return superio_irq;
		}
	}
#endif /* CONFIG_SUPERIO */

	/* lookup IRT entry for isi/slot/pin set */
	irte = iosapic_xlate_pin(isi, pcidev);
	if (NULL == irte) {
		return(-1);
	}
	DBG_IRT("iosapic_fixup_irq(): irte %p %x %x %x %x %x %x %x %x\n",
		irte,
		irte->entry_type,
		irte->entry_length,
		irte->polarity_trigger,
		irte->src_bus_irq_devno,
		irte->src_bus_id,
		irte->src_seg_id,
		irte->dest_iosapic_intin,
		(u32) irte->dest_iosapic_addr);
	isi_line = irte->dest_iosapic_intin;

	/* get vector info for this input line */
	ASSERT(NULL != isi->isi_vector);
	vi = &(isi->isi_vector[isi_line]);
	DBG_IRT("iosapic_fixup_irq:  line %d vi 0x%p\n", isi_line, vi);
	vi->vi_irte = irte;

	/* Allocate processor IRQ */
	vi->vi_txn_irq = txn_alloc_irq();

	if (vi->vi_txn_irq < 0)
		panic("I/O sapic: couldn't get TXN IRQ\n");

	/* enable_irq() will use txn_* to program IRdT */
	vi->vi_txn_addr = txn_alloc_addr(vi->vi_txn_irq);
	vi->vi_txn_data = txn_alloc_data(vi->vi_txn_irq, 8);
        ASSERT(vi->vi_txn_data < 256);  /* matches 8 above */

	tmp = request_irq(vi->vi_txn_irq, iosapic_interrupt, 0,
						vi->vi_name, vi);
	ASSERT(tmp == 0);

	vi->vi_eoi_addr = (u32 *) (isi->isi_hpa + IOSAPIC_REG_EOI);
	vi->vi_eoi_data = cpu_to_le32(vi->vi_irqline);

	ASSERT(NULL != isi->isi_region);
	/* pcidev->irq still needs to be virtualized.  */

	return_irq = isi->isi_region->data.irqbase + isi_line;

#ifdef CONFIG_SUPERIO
	if (superio_irq != -1) {
		superio_inform_irq(return_irq);
		return_irq = superio_irq;
	}
#endif
	pcidev->irq = return_irq;

	DBG_IRT("iosapic_fixup_irq() %d:%d %x %x line %d irq %d\n",
		PCI_SLOT(pcidev->devfn),
	PCI_FUNC(pcidev->devfn), pcidev->vendor, pcidev->device, isi_line, return_irq);

	return return_irq;
}


static void
iosapic_rd_irt_entry(struct vector_info *vi , u32 *dp0, u32 *dp1)
{
	struct iosapic_info *isp = vi->vi_ios;
	u8 idx = vi->vi_irqline;

	/* point the window register to the lower word */
	WRITE_U32(IOSAPIC_IRDT_ENTRY(idx), isp->isi_hpa+IOSAPIC_REG_SELECT);
	*dp0 = READ_U32(isp->isi_hpa+IOSAPIC_REG_WINDOW);

	/* point the window register to the higher word */
	WRITE_U32(IOSAPIC_IRDT_ENTRY_HI(idx), isp->isi_hpa+IOSAPIC_REG_SELECT);
	*dp1 = READ_U32(isp->isi_hpa+IOSAPIC_REG_WINDOW);
}


static void
iosapic_wr_irt_entry(struct vector_info *vi, u32 dp0, u32 dp1)
{
	struct iosapic_info *isp = vi->vi_ios;

	ASSERT(NULL != isp);
	ASSERT(0 != isp->isi_hpa);
	DBG_IRT("iosapic_wr_irt_entry(): irq %d hpa %p WINDOW %p  0x%x 0x%x\n",
		vi->vi_irqline,
		isp->isi_hpa, isp->isi_hpa+IOSAPIC_REG_WINDOW,
		dp0, dp1);

	/* point the window register to the lower word */
	WRITE_U32(IOSAPIC_IRDT_ENTRY(vi->vi_irqline), isp->isi_hpa+IOSAPIC_REG_SELECT);
	WRITE_U32( dp0, isp->isi_hpa+IOSAPIC_REG_WINDOW);

	/* Read the window register to flush the writes down to HW  */
	dp0 = READ_U32(isp->isi_hpa+IOSAPIC_REG_WINDOW);

	/* point the window register to the higher word */
	WRITE_U32(IOSAPIC_IRDT_ENTRY_HI(vi->vi_irqline), isp->isi_hpa+IOSAPIC_REG_SELECT);
	WRITE_U32( dp1, isp->isi_hpa+IOSAPIC_REG_WINDOW);

	/* Read the window register to flush the writes down to HW  */
	dp1 = READ_U32(isp->isi_hpa+IOSAPIC_REG_WINDOW);
}


/*
** set_irt prepares the data (dp0, dp1) according to the vector_info
** and target cpu (id_eid).  dp0/dp1 are then used to program I/O SAPIC
** IRdT for the given "vector" (aka IRQ line).
*/
static void
iosapic_set_irt_data( struct vector_info *vi, u32 *dp0, u32 *dp1)
{
	u32 mode = 0;
	struct irt_entry *p = vi->vi_irte;
	ASSERT(NULL != vi->vi_irte);

	if ((p->polarity_trigger & IRT_PO_MASK) == IRT_ACTIVE_LO)
		mode |= IOSAPIC_IRDT_PO_LOW;

	if (((p->polarity_trigger >> IRT_EL_SHIFT) & IRT_EL_MASK) == IRT_LEVEL_TRIG)
		mode |= IOSAPIC_IRDT_LEVEL_TRIG;

	/*
	** IA64 REVISIT
	** PA doesn't support EXTINT or LPRIO bits.
	*/

	ASSERT(vi->vi_txn_data);
	*dp0 = mode | (u32) vi->vi_txn_data;

	/*
	** Extracting id_eid isn't a real clean way of getting it.
	** But the encoding is the same for both PA and IA64 platforms.
	*/
	if (is_pdc_pat()) {
		/*
		** PAT PDC just hands it to us "right".
		** vi_txn_addr comes from cpu_data[x].txn_addr.
		*/
		*dp1 = (u32) (vi->vi_txn_addr);
	} else {
		/* 
		** eg if base_addr == 0xfffa0000),
		**    we want to get 0xa0ff0000.
		**
		** eid	0x0ff00000 -> 0x00ff0000
		** id	0x000ff000 -> 0xff000000
		*/
		*dp1 = (((u32)vi->vi_txn_addr & 0x0ff00000) >> 4) |
			(((u32)vi->vi_txn_addr & 0x000ff000) << 12);
	}
	DBG_IRT("iosapic_set_irt_data(): 0x%x 0x%x\n", *dp0, *dp1);
}


static void
iosapic_disable_irq(void *irq_dev, int irq)
{
	ulong irqflags;
	struct vector_info *vi = &(((struct vector_info *) irq_dev)[irq]);
	u32 d0, d1;

	ASSERT(NULL != vi);

	IOSAPIC_LOCK(&iosapic_lock);

#ifdef REVISIT_DESIGN_ISSUE
#endif /* REVISIT_DESIGN_ISSUE */

	iosapic_rd_irt_entry(vi, &d0, &d1);
	d0 |= IOSAPIC_IRDT_ENABLE;
	iosapic_wr_irt_entry(vi, d0, d1);

	IOSAPIC_UNLOCK(&iosapic_lock);

	/* disable ISR for parent */
	disable_irq(vi->vi_txn_irq);
}


static void
iosapic_enable_irq(void *dev, int irq)
{
	struct vector_info *vi = &(((struct vector_info *) dev)[irq]);
	u32 d0, d1;

	ASSERT(NULL != vi);
	ASSERT(NULL != vi->vi_irte);

	/* data is initialized by fixup_irq */
	ASSERT(0 < vi->vi_txn_irq);
	ASSERT(0UL != vi->vi_txn_data);

	iosapic_set_irt_data(vi, &d0, &d1);
	iosapic_wr_irt_entry(vi, d0, d1);


#ifdef DEBUG_IOSAPIC_IRT
{
	u32 *t = (u32 *) ((ulong) vi->vi_eoi_addr & ~0xffUL);
	printk("iosapic_enable_irq(): regs %p", vi->vi_eoi_addr);
	while (t < vi->vi_eoi_addr) printk(" %x", READ_U32(t++));
	printk("\n");
}

printk("iosapic_enable_irq(): sel ");
{
	struct iosapic_info *isp = vi->vi_ios;

	for (d0=0x10; d0<0x1e; d0++) {
		/* point the window register to the lower word */
		WRITE_U32(d0, isp->isi_hpa+IOSAPIC_REG_SELECT);

		/* read the word */
		d1 = READ_U32(isp->isi_hpa+IOSAPIC_REG_WINDOW);
		printk(" %x", d1);
	}
}
printk("\n");
#endif

	/*
	** KLUGE: IRQ should not be asserted when Drivers enabling their IRQ.
	**        PCI supports level triggered in order to share IRQ lines.
	**
	** Issueing I/O SAPIC an EOI causes an interrupt iff IRQ line is
	** asserted.
	*/
	IOSAPIC_EOI(vi->vi_eoi_addr, vi->vi_eoi_data);
}


static void
iosapic_mask_irq(void *dev, int irq)
{
	BUG();
}


static void
iosapic_unmask_irq(void *dev, int irq)
{
	BUG();
}


static struct irq_region_ops iosapic_irq_ops = {
	disable_irq:	iosapic_disable_irq,
	enable_irq:	iosapic_enable_irq,
	mask_irq:	iosapic_mask_irq,
	unmask_irq:	iosapic_unmask_irq
};


/*
** squirrel away the I/O Sapic Version
*/
static unsigned int
iosapic_rd_version(struct iosapic_info *isi)
{
	ASSERT(isi);
	ASSERT(isi->isi_hpa);

	/* point window to the version register */
	WRITE_U32(IOSAPIC_REG_VERSION, isi->isi_hpa+IOSAPIC_REG_SELECT);

	/* now read the version register */
	return (READ_U32(isi->isi_hpa+IOSAPIC_REG_WINDOW));
}


/*
** iosapic_register() is called by "drivers" with an integrated I/O SAPIC.
** Caller must be certain they have an I/O SAPIC and know its MMIO address.
**
**	o allocate iosapic_info and add it to the list
**	o read iosapic version and squirrel that away
**	o read size of IRdT.
**	o allocate and initialize isi_vector[]
**	o allocate isi_region (registers region handlers)
*/
void *
iosapic_register(unsigned long hpa)
{
	struct iosapic_info *isi = NULL;
	struct irt_entry *irte = irt_cell;
	struct vector_info *vip;
	int cnt;	/* track how many entries we've looked at */

	/*
	** Astro based platforms can't support PCI OLARD if they
	** implement the legacy PDC (not PAT). Though Legacy PDC
	** supports an IRT, LBA's with no device under them
	** are *not* listed in the IRT.
	** Search the IRT and ignore iosapic's which aren't
	** in the IRT.
	*/
	ASSERT(NULL != irte);	/* always have built-in devices */
	for (cnt=0; cnt < irt_num_entry; cnt++, irte++) {
		ASSERT(IRT_IOSAPIC_TYPE == irte->entry_type);
		/*
		** We need sign extension of the hpa on 32-bit kernels.
		** The address in the IRT is *always* 64 bit and really
		** is an unsigned quantity (like all physical addresses).
		*/ 
		if (irte->dest_iosapic_addr == (s64) ((long) hpa))
			break;
	}

	if (cnt  >= irt_num_entry)
		return (NULL);

	if ((isi = IOSAPIC_KALLOC(struct iosapic_info, 1)) == NULL) {
		BUG();
		return (NULL);
	}

	memset(isi, 0, sizeof(struct iosapic_info));

	isi->isi_hpa         = hpa;
	isi->isi_version     = iosapic_rd_version(isi);
	isi->isi_num_vectors = IOSAPIC_IRDT_MAX_ENTRY(isi->isi_version) + 1;

	vip = isi->isi_vector =
		 IOSAPIC_KALLOC(struct vector_info, isi->isi_num_vectors);

	if (vip == NULL) {
		IOSAPIC_FREE(isi, struct iosapic_info, 1);
		return (NULL);
	}

	memset(vip, 0, sizeof(struct vector_info) * isi->isi_num_vectors);
	sprintf(isi->isi_name, "IO-SAPIC%02d", iosapic_count++);

	/*
	** Initialize vector array
	*/
	for (cnt=0; cnt < isi->isi_num_vectors; cnt++, vip++) {
		vip->vi_irqline = (unsigned char) cnt;
		vip->vi_ios = isi;
		sprintf(vip->vi_name, "%s-L%d", isi->isi_name, cnt);
	}

	isi->isi_region = alloc_irq_region(isi->isi_num_vectors,
				&iosapic_irq_ops, isi->isi_name,
				(void *) isi->isi_vector);

	ASSERT(NULL != isi->isi_region);
	return ((void *) isi);
}


#ifdef DEBUG_IOSAPIC

static void
iosapic_prt_irt(void *irt, long num_entry)
{
	unsigned int i, *irp = (unsigned int *) irt;

	ASSERT(NULL != irt);

	printk(KERN_DEBUG MODULE_NAME ": Interrupt Routing Table (%lx entries)\n", num_entry);

	for (i=0; i<num_entry; i++, irp += 4) {
		printk(KERN_DEBUG "%p : %2d %.8x %.8x %.8x %.8x\n",
					irp, i, irp[0], irp[1], irp[2], irp[3]);
	}
}


static void
iosapic_prt_vi(struct vector_info *vi)
{
	ASSERT(NULL != vi);

	printk(KERN_DEBUG MODULE_NAME ": vector_info[%d] is at %p\n", vi->vi_irqline, vi);
	printk(KERN_DEBUG "\t\tvi_status:	 %.4x\n", vi->vi_status);
	printk(KERN_DEBUG "\t\tvi_txn_irq:  %d\n",  vi->vi_txn_irq);
	printk(KERN_DEBUG "\t\tvi_txn_addr: %lx\n", vi->vi_txn_addr);
	printk(KERN_DEBUG "\t\tvi_txn_data: %lx\n", vi->vi_txn_data);
	printk(KERN_DEBUG "\t\tvi_eoi_addr: %p\n",  vi->vi_eoi_addr);
	printk(KERN_DEBUG "\t\tvi_eoi_data: %x\n",  vi->vi_eoi_data);
}


static void
iosapic_prt_isi(struct iosapic_info *isi)
{
	ASSERT(NULL != isi);
	printk(KERN_DEBUG MODULE_NAME ": io_sapic_info at %p\n", isi);
	printk(KERN_DEBUG "\t\tisi_hpa:       %lx\n", isi->isi_hpa);
	printk(KERN_DEBUG "\t\tisi_status:     %x\n", isi->isi_status);
	printk(KERN_DEBUG "\t\tisi_version:   %x\n", isi->isi_version);
	printk(KERN_DEBUG "\t\tisi_vector:    %p\n", isi->isi_vector);
}
#endif /* DEBUG_IOSAPIC */
