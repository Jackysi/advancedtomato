/*
 * pci.c - Low-Level PCI Access in IA-64
 *
 * Derived from bios32.c of i386 tree.
 */
#include <linux/config.h>

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/slab.h>
#include <linux/smp_lock.h>
#include <linux/spinlock.h>
#include <linux/acpi.h>

#include <asm/machvec.h>
#include <asm/page.h>
#include <asm/segment.h>
#include <asm/system.h>
#include <asm/io.h>

#include <asm/sal.h>


#ifdef CONFIG_SMP
# include <asm/smp.h>
#endif
#include <asm/irq.h>


#undef DEBUG
#define DEBUG

#ifdef DEBUG
#define DBG(x...) printk(x)
#else
#define DBG(x...)
#endif

#ifdef CONFIG_IA64_MCA
extern void ia64_mca_check_errors( void );
#endif

struct pci_fixup pcibios_fixups[1];

struct pci_ops *pci_root_ops;

int (*pci_config_read)(int seg, int bus, int dev, int fn, int reg, int len, u32 *value);
int (*pci_config_write)(int seg, int bus, int dev, int fn, int reg, int len, u32 value);


/*
 * Low-level SAL-based PCI configuration access functions. Note that SAL
 * calls are already serialized (via sal_lock), so we don't need another
 * synchronization mechanism here.
 */

#define PCI_SAL_ADDRESS(seg, bus, dev, fn, reg) \
	((u64)(seg << 24) | (u64)(bus << 16) | \
	 (u64)(dev << 11) | (u64)(fn << 8) | (u64)(reg))

static int
pci_sal_read (int seg, int bus, int dev, int fn, int reg, int len, u32 *value)
{
	int result = 0;
	u64 data = 0;

	if (!value || (seg > 255) || (bus > 255) || (dev > 31) || (fn > 7) || (reg > 255))
		return -EINVAL;

	result = ia64_sal_pci_config_read(PCI_SAL_ADDRESS(seg, bus, dev, fn, reg), len, &data);

	*value = (u32) data;

	return result;
}

static int
pci_sal_write (int seg, int bus, int dev, int fn, int reg, int len, u32 value)
{
	if ((seg > 255) || (bus > 255) || (dev > 31) || (fn > 7) || (reg > 255))
		return -EINVAL;

	return ia64_sal_pci_config_write(PCI_SAL_ADDRESS(seg, bus, dev, fn, reg), len, value);
}


static int
pci_sal_read_config_byte (struct pci_dev *dev, int where, u8 *value)
{
	int result = 0;
	u32 data = 0;

	if (!value)
		return -EINVAL;

	result = pci_sal_read(PCI_SEGMENT(dev), dev->bus->number, PCI_SLOT(dev->devfn),
			      PCI_FUNC(dev->devfn), where, 1, &data);

	*value = (u8) data;

	return result;
}

static int
pci_sal_read_config_word (struct pci_dev *dev, int where, u16 *value)
{
	int result = 0;
	u32 data = 0;

	if (!value)
		return -EINVAL;

	result = pci_sal_read(PCI_SEGMENT(dev), dev->bus->number, PCI_SLOT(dev->devfn),
			      PCI_FUNC(dev->devfn), where, 2, &data);

	*value = (u16) data;

	return result;
}

static int
pci_sal_read_config_dword (struct pci_dev *dev, int where, u32 *value)
{
	if (!value)
		return -EINVAL;

	return pci_sal_read(PCI_SEGMENT(dev), dev->bus->number, PCI_SLOT(dev->devfn),
			    PCI_FUNC(dev->devfn), where, 4, value);
}

static int
pci_sal_write_config_byte (struct pci_dev *dev, int where, u8 value)
{
	return pci_sal_write(PCI_SEGMENT(dev), dev->bus->number, PCI_SLOT(dev->devfn),
			     PCI_FUNC(dev->devfn), where, 1, value);
}

static int
pci_sal_write_config_word (struct pci_dev *dev, int where, u16 value)
{
	return pci_sal_write(PCI_SEGMENT(dev), dev->bus->number, PCI_SLOT(dev->devfn),
			     PCI_FUNC(dev->devfn), where, 2, value);
}

static int
pci_sal_write_config_dword (struct pci_dev *dev, int where, u32 value)
{
	return pci_sal_write(PCI_SEGMENT(dev), dev->bus->number, PCI_SLOT(dev->devfn),
			     PCI_FUNC(dev->devfn), where, 4, value);
}

struct pci_ops pci_sal_ops = {
	pci_sal_read_config_byte,
	pci_sal_read_config_word,
	pci_sal_read_config_dword,
	pci_sal_write_config_byte,
	pci_sal_write_config_word,
	pci_sal_write_config_dword
};


/*
 * Initialization. Uses the SAL interface
 */

static struct pci_controller *
alloc_pci_controller(int seg)
{
	struct pci_controller *controller;

	controller = kmalloc(sizeof(*controller), GFP_KERNEL);
	if (!controller)
		return NULL;

	memset(controller, 0, sizeof(*controller));
	controller->segment = seg;
	return controller;
}

static struct pci_bus *
scan_root_bus(int bus, struct pci_ops *ops, void *sysdata)
{
	struct pci_bus *b;

	/*
	 * We know this is a new root bus we haven't seen before, so
	 * scan it, even if we've seen the same bus number in a different
	 * segment.
	 */
	b = kmalloc(sizeof(*b), GFP_KERNEL);
	if (!b)
		return NULL;

	memset(b, 0, sizeof(*b));
	INIT_LIST_HEAD(&b->children);
	INIT_LIST_HEAD(&b->devices);

	list_add_tail(&b->node, &pci_root_buses);

	b->number = b->secondary = bus;
	b->resource[0] = &ioport_resource;
	b->resource[1] = &iomem_resource;

	b->sysdata = sysdata;
	b->ops = ops;
	b->subordinate = pci_do_scan_bus(b);

	return b;
}

struct pci_bus *
pcibios_scan_root(void *handle, int seg, int bus)
{
	struct pci_controller *controller;
	u64 base, size, offset;

	printk("PCI: Probing PCI hardware on bus (%02x:%02x)\n", seg, bus);

	controller = alloc_pci_controller(seg);
	if (!controller)
		return NULL;

	controller->acpi_handle = handle;

	acpi_get_addr_space(handle, ACPI_MEMORY_RANGE, &base, &size, &offset);
	controller->mem_offset = offset;

	return scan_root_bus(bus, pci_root_ops, controller);
}

void __init
pcibios_config_init (void)
{
	if (pci_root_ops)
		return;

	printk("PCI: Using SAL to access configuration space\n");

	pci_root_ops = &pci_sal_ops;
	pci_config_read = pci_sal_read;
	pci_config_write = pci_sal_write;

	return;
}

void __init
pcibios_init (void)
{
#	define PCI_BUSES_TO_SCAN 255
	int i = 0;
	struct pci_controller *controller;

#ifdef CONFIG_IA64_MCA
	ia64_mca_check_errors();    /* For post-failure MCA error logging */
#endif

	pcibios_config_init();

	platform_pci_fixup(0);	/* phase 0 fixups (before buses scanned) */

	printk("PCI: Probing PCI hardware\n");
	controller = alloc_pci_controller(0);
	if (controller)
		for (i = 0; i < PCI_BUSES_TO_SCAN; i++)
			pci_scan_bus(i, pci_root_ops, controller);

	platform_pci_fixup(1);	/* phase 1 fixups (after buses scanned) */

	return;
}

static void __init
pcibios_fixup_resource(struct resource *res, u64 offset)
{
	res->start += offset;
	res->end += offset;
}

void __init
pcibios_fixup_device_resources(struct pci_dev *dev, struct pci_bus *bus)
{
	int i;

	for (i = 0; i < PCI_NUM_RESOURCES; i++) {
		if (!dev->resource[i].start)
			continue;
		if (dev->resource[i].flags & IORESOURCE_MEM)
			pcibios_fixup_resource(&dev->resource[i],
				PCI_CONTROLLER(dev)->mem_offset);
	}
}

/*
 *  Called after each bus is probed, but before its children
 *  are examined.
 */
void __init
pcibios_fixup_bus (struct pci_bus *b)
{
	struct list_head *ln;

	for (ln = b->devices.next; ln != &b->devices; ln = ln->next)
		pcibios_fixup_device_resources(pci_dev_b(ln), b);
}

void __init
pcibios_update_resource (struct pci_dev *dev, struct resource *root,
			 struct resource *res, int resource)
{
	unsigned long where, size;
	u32 reg;

	where = PCI_BASE_ADDRESS_0 + (resource * 4);
	size = res->end - res->start;
	pci_read_config_dword(dev, where, &reg);
	reg = (reg & size) | (((u32)(res->start - root->start)) & ~size);
	pci_write_config_dword(dev, where, reg);

}

void __init
pcibios_update_irq (struct pci_dev *dev, int irq)
{
	pci_write_config_byte(dev, PCI_INTERRUPT_LINE, irq);

}

void __init
pcibios_fixup_pbus_ranges (struct pci_bus * bus, struct pbus_set_ranges_data * ranges)
{
	ranges->io_start -= bus->resource[0]->start;
	ranges->io_end -= bus->resource[0]->start;
	ranges->mem_start -= bus->resource[1]->start;
	ranges->mem_end -= bus->resource[1]->start;
}

int
pcibios_enable_device (struct pci_dev *dev)
{
	u16 cmd, old_cmd;
	int idx;
	struct resource *r;

	if (!dev)
		return -EINVAL;

 	platform_pci_enable_device(dev);

	pci_read_config_word(dev, PCI_COMMAND, &cmd);
	old_cmd = cmd;
	for (idx=0; idx<6; idx++) {
		r = &dev->resource[idx];
		if (!r->start && r->end) {
			printk(KERN_ERR
			       "PCI: Device %s not available because of resource collisions\n",
			       dev->slot_name);
			return -EINVAL;
		}
		if (r->flags & IORESOURCE_IO)
			cmd |= PCI_COMMAND_IO;
		if (r->flags & IORESOURCE_MEM)
			cmd |= PCI_COMMAND_MEMORY;
	}
	if (dev->resource[PCI_ROM_RESOURCE].start)
		cmd |= PCI_COMMAND_MEMORY;
	if (cmd != old_cmd) {
		printk("PCI: Enabling device %s (%04x -> %04x)\n", dev->slot_name, old_cmd, cmd);
		pci_write_config_word(dev, PCI_COMMAND, cmd);
	}

	printk(KERN_INFO "PCI: Found IRQ %d for device %s\n", dev->irq, dev->slot_name);

	return 0;
}

void
pcibios_align_resource (void *data, struct resource *res,
		        unsigned long size, unsigned long align)
{
}

/*
 * PCI BIOS setup, always defaults to SAL interface
 */
char * __init
pcibios_setup (char *str)
{
	return NULL;
}

int
pci_mmap_page_range (struct pci_dev *dev, struct vm_area_struct *vma,
		     enum pci_mmap_state mmap_state, int write_combine)
{
	/*
	 * I/O space cannot be accessed via normal processor loads and stores on this
	 * platform.
	 */
	if (mmap_state == pci_mmap_io)
		return -EINVAL;

	/*
	 * Leave vm_pgoff as-is, the PCI space address is the physical address on this
	 * platform.
	 */
	vma->vm_flags |= (VM_SHM | VM_LOCKED | VM_IO);

	if (write_combine)
		vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
	else
		vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	if (remap_page_range(vma->vm_start, vma->vm_pgoff << PAGE_SHIFT,
			     vma->vm_end - vma->vm_start,
			     vma->vm_page_prot))
		return -EAGAIN;

	return 0;
}
