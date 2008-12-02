#ifdef DEBUG
static void __attribute__((__unused__)) uhci_show_qh (puhci_desc_t qh)
{
	if (qh->type != QH_TYPE) {
		dbg("qh has not QH_TYPE");
		return;
	}
	dbg("QH @ %p/%08llX:", qh, (unsigned long long)qh->dma_addr);

	if (qh->hw.qh.head & UHCI_PTR_TERM)
		dbg("    Head Terminate");
	else 
		dbg("    Head: %s @ %08X",
		    (qh->hw.qh.head & UHCI_PTR_QH?"QH":"TD"),
		    qh->hw.qh.head & ~UHCI_PTR_BITS);

	if (qh->hw.qh.element & UHCI_PTR_TERM)
		dbg("    Element Terminate");
	else 
		dbg("    Element: %s @ %08X",
		    (qh->hw.qh.element & UHCI_PTR_QH?"QH":"TD"),
		    qh->hw.qh.element & ~UHCI_PTR_BITS);
}
#endif


#ifdef DEBUG
static void __attribute__((__unused__)) uhci_show_td_queue (puhci_desc_t td)
{
	//dbg("uhci_show_td_queue %p (%08lX):", td, td->dma_addr);
	return;
}

static void __attribute__((__unused__)) uhci_show_queue (puhci_desc_t qh)
{

	dbg("uhci_show_queue %p:", qh);
	return;
}

static void __attribute__((__unused__)) uhci_show_sc (int port, unsigned short status)
{
	dbg("  stat%d     =     %04x   %s%s%s%s%s%s%s%s",
	     port,
	     status,
	     (status & USBPORTSC_SUSP) ? "PortSuspend " : "",
	     (status & USBPORTSC_PR) ? "PortReset " : "",
	     (status & USBPORTSC_LSDA) ? "LowSpeed " : "",
	     (status & USBPORTSC_RD) ? "ResumeDetect " : "",
	     (status & USBPORTSC_PEC) ? "EnableChange " : "",
	     (status & USBPORTSC_PE) ? "PortEnabled " : "",
	     (status & USBPORTSC_CSC) ? "ConnectChange " : "",
	     (status & USBPORTSC_CCS) ? "PortConnected " : "");
}

void uhci_show_status (puhci_t s)
{
	unsigned int io_addr = s->io_addr;
	unsigned short usbcmd, usbstat, usbint, usbfrnum;
	unsigned int flbaseadd;
	unsigned char sof;
	unsigned short portsc1, portsc2;

	usbcmd = inw (io_addr + 0);
	usbstat = inw (io_addr + 2);
	usbint = inw (io_addr + 4);
	usbfrnum = inw (io_addr + 6);
	flbaseadd = inl (io_addr + 8);
	sof = inb (io_addr + 12);
	portsc1 = inw (io_addr + 16);
	portsc2 = inw (io_addr + 18);

	dbg("  usbcmd    =     %04x   %s%s%s%s%s%s%s%s",
	     usbcmd,
	     (usbcmd & USBCMD_MAXP) ? "Maxp64 " : "Maxp32 ",
	     (usbcmd & USBCMD_CF) ? "CF " : "",
	     (usbcmd & USBCMD_SWDBG) ? "SWDBG " : "",
	     (usbcmd & USBCMD_FGR) ? "FGR " : "",
	     (usbcmd & USBCMD_EGSM) ? "EGSM " : "",
	     (usbcmd & USBCMD_GRESET) ? "GRESET " : "",
	     (usbcmd & USBCMD_HCRESET) ? "HCRESET " : "",
	     (usbcmd & USBCMD_RS) ? "RS " : "");

	dbg("  usbstat   =     %04x   %s%s%s%s%s%s",
	     usbstat,
	     (usbstat & USBSTS_HCH) ? "HCHalted " : "",
	     (usbstat & USBSTS_HCPE) ? "HostControllerProcessError " : "",
	     (usbstat & USBSTS_HSE) ? "HostSystemError " : "",
	     (usbstat & USBSTS_RD) ? "ResumeDetect " : "",
	     (usbstat & USBSTS_ERROR) ? "USBError " : "",
	     (usbstat & USBSTS_USBINT) ? "USBINT " : "");

	dbg("  usbint    =     %04x", usbint);
	dbg("  usbfrnum  =   (%d)%03x", (usbfrnum >> 10) & 1,
	     0xfff & (4 * (unsigned int) usbfrnum));
	dbg("  flbaseadd = %08x", flbaseadd);
	dbg("  sof       =       %02x", sof);
	uhci_show_sc (1, portsc1);
	uhci_show_sc (2, portsc2);
}
#endif
