#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <asm/bootinfo.h>

#include <asm/lasat/lasat.h>
#include <asm/gt64120.h>
#include <asm/nile4.h>

#define PCI_ACCESS_READ  0
#define PCI_ACCESS_WRITE 1

#undef DEBUG_PCI
#ifdef DEBUG_PCI
#define Dprintk(fmt...) printk(fmt)
#else
#define Dprintk(fmt...)
#endif

static int (*lasat_pcibios_config_access)(unsigned char access_type,
				       struct pci_dev *dev,
				       unsigned char reg,
				       u32 *data);

/*
 * Because of an error/peculiarity in the Galileo chip, we need to swap the 
 * bytes when running bigendian.
 */
#define GT_WRITE(ofs, data)  \
             *(volatile u32 *)(LASAT_GT_BASE+ofs) = cpu_to_le32(data)
#define GT_READ(ofs, data)   \
             data = le32_to_cpu(*(volatile u32 *)(LASAT_GT_BASE+ofs))


static int lasat_pcibios_config_access_100(unsigned char access_type,
				       struct pci_dev *dev,
				       unsigned char reg,
				       u32 *data)
{
	unsigned char bus = dev->bus->number;
	unsigned char dev_fn = dev->devfn;
        u32 intr;

	if ((bus == 0) && (dev_fn >= PCI_DEVFN(31,0)))
	        return -1; /* Because of a bug in the Galileo (for slot 31). */

	/* Clear cause register bits */
	GT_WRITE( GT_INTRCAUSE_OFS, ~(GT_INTRCAUSE_MASABORT0_BIT | 
				      GT_INTRCAUSE_TARABORT0_BIT) );

	/* Setup address */
	GT_WRITE( GT_PCI0_CFGADDR_OFS, 
		  (bus       << GT_PCI0_CFGADDR_BUSNUM_SHF) |
		  (dev_fn    << GT_PCI0_CFGADDR_FUNCTNUM_SHF) |
		  ((reg / 4) << GT_PCI0_CFGADDR_REGNUM_SHF)   |
		  GT_PCI0_CFGADDR_CONFIGEN_BIT );

	if (access_type == PCI_ACCESS_WRITE) {
	        GT_WRITE( GT_PCI0_CFGDATA_OFS, *data );
	} else {
	        GT_READ( GT_PCI0_CFGDATA_OFS, *data );
	}

	/* Check for master or target abort */
	GT_READ( GT_INTRCAUSE_OFS, intr );

	if( intr & (GT_INTRCAUSE_MASABORT0_BIT | GT_INTRCAUSE_TARABORT0_BIT) )
	{
	        /* Error occured */

	        /* Clear bits */
	        GT_WRITE( GT_INTRCAUSE_OFS, ~(GT_INTRCAUSE_MASABORT0_BIT | 
					      GT_INTRCAUSE_TARABORT0_BIT) );

		return -1;
	}

	return 0;
}

#define LO(reg) (reg / 4)
#define HI(reg) (reg / 4 + 1)

volatile unsigned long * const vrc_pciregs = (void *)Vrc5074_BASE;

static int lasat_pcibios_config_access_200(unsigned char access_type,
				       struct pci_dev *dev,
				       unsigned char reg,
				       u32 *data)
{
	unsigned char bus = dev->bus->number;
	unsigned char dev_fn = dev->devfn;
	u32 adr, mask, err;

	if ((bus == 0) && (PCI_SLOT(dev_fn) > 8))
		/* The addressing scheme chosen leaves room for just
		 * 8 devices on the first bus (besides the PCI
		 * controller itself) */
		return -1;

	if ((bus == 0) && (dev_fn == PCI_DEVFN(0,0))) {
		/* Access controller registers directly */
		if (access_type == PCI_ACCESS_WRITE) {
			vrc_pciregs[(0x200+reg) >> 2] = *data;
		} else {
			*data = vrc_pciregs[(0x200+reg) >> 2];
		}
	        return 0;
	}

	/* Temporarily map PCI Window 1 to config space */
	mask = vrc_pciregs[LO(NILE4_PCIINIT1)];
	vrc_pciregs[LO(NILE4_PCIINIT1)] = 0x0000001a | (bus ? 0x200 : 0);

	/* Clear PCI Error register. This also clears the Error Type
	 * bits in the Control register */
	vrc_pciregs[LO(NILE4_PCIERR)] = 0;
	vrc_pciregs[HI(NILE4_PCIERR)] = 0;

	/* Setup address */
	if (bus == 0)
	        adr = KSEG1ADDR(PCI_WINDOW1) + ((1 << (PCI_SLOT(dev_fn) + 15)) | (PCI_FUNC(dev_fn) << 8) | (reg & ~3));
	else
	        adr = KSEG1ADDR(PCI_WINDOW1) | (bus << 16) | (dev_fn << 8) | (reg & ~3);

#ifdef DEBUG_PCI
	printk("PCI config %s: adr %x", access_type == PCI_ACCESS_WRITE ? "write" : "read", adr);
#endif

	if (access_type == PCI_ACCESS_WRITE) {
	        *(u32 *)adr = *data;
	} else {
	        *data = *(u32 *)adr;
	}

#ifdef DEBUG_PCI
	printk(" value = %x\n", *data);
#endif

	/* Check for master or target abort */
	err = (vrc_pciregs[HI(NILE4_PCICTRL)] >> 5) & 0x7;

	/* Restore PCI Window 1 */
	vrc_pciregs[LO(NILE4_PCIINIT1)] = mask;

	if (err)
	{
		/* Error occured */
#ifdef DEBUG_PCI
	        printk("\terror %x at adr %x\n", err, vrc_pciregs[LO(PCIERR)]);
#endif
		return -1;
	}

	return 0;
}

/*
 * We can't address 8 and 16 bit words directly.  Instead we have to
 * read/write a 32bit word and mask/modify the data we actually want.
 */
static int lasat_pcibios_read_config_byte(struct pci_dev *dev, int reg, u8 *val)
{
        u32 data=0, flags;

	save_and_cli(flags);

	if (lasat_pcibios_config_access(PCI_ACCESS_READ, dev, reg, &data)) {
		restore_flags(flags);
		return -1;
	}

	*val = (data >> ((reg & 3) << 3)) & 0xff;

	restore_flags(flags);
	return PCIBIOS_SUCCESSFUL;
}


static int lasat_pcibios_read_config_word(struct pci_dev *dev, int reg, u16 *val)
{
        u32 data=0, flags;

	if (reg & 1)
		return PCIBIOS_BAD_REGISTER_NUMBER;

	save_and_cli(flags);

	if (lasat_pcibios_config_access(PCI_ACCESS_READ, dev, reg, &data)) {
		restore_flags(flags);
		return -1;
	}

	*val = (data >> ((reg & 3) << 3)) & 0xffff;

	restore_flags(flags);
	return PCIBIOS_SUCCESSFUL;
}

static int lasat_pcibios_read_config_dword(struct pci_dev *dev, int reg, u32 *val)
{
        u32 data=0, flags;

	if (reg & 3)
		return PCIBIOS_BAD_REGISTER_NUMBER;

	save_and_cli(flags);

	if (lasat_pcibios_config_access(PCI_ACCESS_READ, dev, reg, &data)) {
		restore_flags(flags);
		return -1;
	}

	*val = data;

	restore_flags(flags);
	return PCIBIOS_SUCCESSFUL;
}


static int lasat_pcibios_write_config_byte(struct pci_dev *dev, int reg, u8 val)
{
        u32 data=0, flags, err;

	save_and_cli(flags);

        err = lasat_pcibios_config_access(PCI_ACCESS_READ, dev, reg, &data);
        if (err)
		goto out;

	data = (data & ~(0xff << ((reg & 3) << 3))) |
		(val << ((reg & 3) << 3));

	err = lasat_pcibios_config_access(PCI_ACCESS_WRITE, dev, reg, &data);
out:
	restore_flags(flags);

	if (err)
		return -1;
	else
		return PCIBIOS_SUCCESSFUL;
}

static int lasat_pcibios_write_config_word(struct pci_dev *dev, int reg, u16 val)
{
	u32 data=0, flags, err;

	if (reg & 1)
		return PCIBIOS_BAD_REGISTER_NUMBER;
       
	save_and_cli(flags);

        err = lasat_pcibios_config_access(PCI_ACCESS_READ, dev, reg, &data);
        if (err)
	       goto out;

	data = (data & ~(0xffff << ((reg & 3) << 3))) | 
	       (val << ((reg & 3) << 3));

	err = lasat_pcibios_config_access(PCI_ACCESS_WRITE, dev, reg, &data);
out:
	restore_flags(flags);

	if (err)
		return -1;
	else
		return PCIBIOS_SUCCESSFUL;
}

static int lasat_pcibios_write_config_dword(struct pci_dev *dev, int reg, u32 val)
{
        u32 flags, err;

	if (reg & 3)
		return PCIBIOS_BAD_REGISTER_NUMBER;

	save_and_cli(flags);

	err = lasat_pcibios_config_access(PCI_ACCESS_WRITE, dev, reg, &val);
	restore_flags(flags);

	if (err)
		return -1;
	else
		return PCIBIOS_SUCCESSFUL;
}

struct pci_ops lasat_pci_ops = {
	lasat_pcibios_read_config_byte,
	lasat_pcibios_read_config_word,
	lasat_pcibios_read_config_dword,
	lasat_pcibios_write_config_byte,
	lasat_pcibios_write_config_word,
	lasat_pcibios_write_config_dword
};

char * __init pcibios_setup(char *str)
{
	return str;
}

void __init pcibios_init(void)
{
	switch (mips_machtype) {
	case MACH_LASAT_100:
		lasat_pcibios_config_access = &lasat_pcibios_config_access_100;
		break;
	case MACH_LASAT_200:
		lasat_pcibios_config_access = &lasat_pcibios_config_access_200;
		break;
	default:
		panic("pcibios_init: mips_machtype incorrect");
	}

	Dprintk("pcibios_init()\n");
	pci_scan_bus(0, &lasat_pci_ops, NULL);
}

void __init pcibios_fixup_bus(struct pci_bus *b)
{
	Dprintk("pcibios_fixup_bus()\n");
}

void pcibios_update_resource(struct pci_dev *dev, struct resource *root,
			     struct resource *res, int resource)
{
}

int __init pcibios_enable_device(struct pci_dev *dev, int mask)
{
	/* Not needed, since we enable all devices at startup.  */
	return 0;
}

void __init pcibios_align_resource(void *data, struct resource *res,
	unsigned long size, unsigned long align)
{
}

unsigned __init int pcibios_assign_all_busses(void)
{
	return 1;
}

struct pci_fixup pcibios_fixups[] = {
	{ 0 }
};
