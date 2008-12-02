/*
 * BRIEF MODULE DESCRIPTION
 *	Pb1500 specific pci support.
 *
 * Copyright 2001,2002 MontaVista Software Inc.
 * Author: MontaVista Software, Inc.
 *         	ppopov@mvista.com or source@mvista.com
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 *  THIS  SOFTWARE  IS PROVIDED   ``AS  IS'' AND   ANY  EXPRESS OR IMPLIED
 *  WARRANTIES,   INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 *  NO  EVENT  SHALL   THE AUTHOR  BE    LIABLE FOR ANY   DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED   TO, PROCUREMENT OF  SUBSTITUTE GOODS  OR SERVICES; LOSS OF
 *  USE, DATA,  OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  You should have received a copy of the  GNU General Public License along
 *  with this program; if not, write  to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <linux/config.h>

#ifdef CONFIG_PCI

#include <linux/types.h>
#include <linux/pci.h>
#include <linux/kernel.h>
#include <linux/init.h>

#include <asm/au1000.h>
#include <asm/pb1500.h>
#include <asm/pci_channel.h>

#define PCI_ACCESS_READ  0
#define PCI_ACCESS_WRITE 1

#undef DEBUG
#ifdef 	DEBUG
#define	DBG(x...)	printk(x)
#else
#define	DBG(x...)	
#endif

#ifndef CONFIG_CPU_AU1500
struct pci_channel mips_pci_channels[] = {
    {(struct pci_ops *) NULL, (struct resource *) NULL,
     (struct resource *) NULL, (int) NULL, (int) NULL}
};
#else

/* TBD */
static struct resource pci_io_resource = {
	"pci IO space", 
	Au1500_PCI_IO_START,
	Au1500_PCI_IO_END,
	IORESOURCE_IO
};

static struct resource pci_mem_resource = {
	"pci memory space", 
	Au1500_PCI_MEM_START,
	Au1500_PCI_MEM_END,
	IORESOURCE_MEM
};

extern struct pci_ops pb1500_pci_ops;

struct pci_channel mips_pci_channels[] = {
	{&pb1500_pci_ops, &pci_io_resource, &pci_mem_resource, (10<<3),(16<<3)},
	{(struct pci_ops *) NULL, (struct resource *) NULL,
	 (struct resource *) NULL, (int) NULL, (int) NULL}
};

static unsigned long cfg_addr;
static int config_access(unsigned char access_type, struct pci_dev *dev, 
			 unsigned char where, u32 * data)
{
	unsigned char bus = dev->bus->number;
	unsigned int dev_fn = dev->devfn;
	unsigned int device, function;
	unsigned long config, status;
	static int first = 1;

	/* 
 	 * 7:3 = slot
 	 * 2:0 = function
	 */

	if (bus != 0) {
		*data = 0xffffffff;
		return -1;
	}

	if (first) {
		first = 0;
		cfg_addr = ioremap(Au1500_EXT_CFG, 0x10000000);
		if (!cfg_addr) 
			printk (KERN_ERR "PCI unable to ioremap cfg space\n");
	}

	device = (dev_fn >> 3) & 0x1f;
	function = dev_fn & 0x7;

	//if (!cfg_addr || (device < 10) || (device > 16)) {
	if (!cfg_addr || (device > 16)) {
		*data = 0xffffffff;
		return -1;
	}

	au_writel(((0x2000 << 16) | (au_readl(Au1500_PCI_STATCMD) & 0xffff)),
			Au1500_PCI_STATCMD);
	//au_writel(au_readl(Au1500_PCI_CFG) & ~PCI_ERROR, Au1500_PCI_CFG);
	au_sync_udelay(1);

	/* setup the lower 31 bits of the 36 bit address */
	config = cfg_addr | 
		((1<<device)<<11) | (function << 8) | (where & ~0x3);


	if (access_type == PCI_ACCESS_WRITE) {
		au_writel(*data, config);
	} else {
		*data = au_readl(config);
	}
	au_sync_udelay(1);

	DBG("config_access: %d bus %d device %d at %x *data %x, conf %x\n", 
			access_type, bus, device, where, *data, config);

	/* check master abort */
	status = au_readl(Au1500_PCI_STATCMD);
	if (status & (1<<29)) { 
		*data = 0xffffffff;
		return -1;
	} else if ((status >> 28) & 0xf) {
		DBG("PCI ERR detected: status %x\n", status);
		*data = 0xffffffff;
		return -1;
	}
	else {
		return PCIBIOS_SUCCESSFUL;
	}
}


static int read_config_byte(struct pci_dev *dev, int where, u8 * val)
{
	u32 data;
	int ret;

	ret = config_access(PCI_ACCESS_READ, dev, where, &data);
        if (where & 1) data >>= 8;
        if (where & 2) data >>= 16;
        *val = data & 0xff;
	return ret; 
}


static int read_config_word(struct pci_dev *dev, int where, u16 * val)
{
	u32 data;
	int ret;

	ret = config_access(PCI_ACCESS_READ, dev, where, &data);
        if (where & 2) data >>= 16;
        *val = data & 0xffff;
	return ret; 
}

static int read_config_dword(struct pci_dev *dev, int where, u32 * val)
{
	int ret;

	ret = config_access(PCI_ACCESS_READ, dev, where, val);
	return ret; 
}


static int write_config_byte(struct pci_dev *dev, int where, u8 val)
{
	u32 data = 0;
       
	if (config_access(PCI_ACCESS_READ, dev, where, &data))
		return -1;

	data = (data & ~(0xff << ((where & 3) << 3))) |
	       (val << ((where & 3) << 3));

	if (config_access(PCI_ACCESS_WRITE, dev, where, &data))
		return -1;

	return PCIBIOS_SUCCESSFUL;
}

static int write_config_word(struct pci_dev *dev, int where, u16 val)
{
        u32 data = 0;

	if (where & 1)
		return PCIBIOS_BAD_REGISTER_NUMBER;
       
        if (config_access(PCI_ACCESS_READ, dev, where, &data))
	       return -1;

	data = (data & ~(0xffff << ((where & 3) << 3))) | 
	       (val << ((where & 3) << 3));

	if (config_access(PCI_ACCESS_WRITE, dev, where, &data))
	       return -1;


	return PCIBIOS_SUCCESSFUL;
}

static int write_config_dword(struct pci_dev *dev, int where, u32 val)
{
	if (where & 3)
		return PCIBIOS_BAD_REGISTER_NUMBER;

	if (config_access(PCI_ACCESS_WRITE, dev, where, &val))
	       return -1;

	return PCIBIOS_SUCCESSFUL;
}

struct pci_ops pb1500_pci_ops = {
	read_config_byte,
        read_config_word,
	read_config_dword,
	write_config_byte,
	write_config_word,
	write_config_dword
};

#endif // CONFIG_CPU_AU1500
#endif /* CONFIG_PCI */

