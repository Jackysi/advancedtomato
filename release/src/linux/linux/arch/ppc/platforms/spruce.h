/*
 * include/asm-ppc/platforms/spruce.h
 * 
 * Definitions for IBM Spruce reference board support
 *
 * Authors: Matt Porter and Johnnie Peters
 *          mporter@mvista.com
 *          jpeters@mvista.com
 *
 * Copyright 2001 MontaVista Software Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 * THIS  SOFTWARE  IS PROVIDED   ``AS  IS'' AND   ANY  EXPRESS OR   IMPLIED
 * WARRANTIES,   INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 * NO  EVENT  SHALL   THE AUTHOR  BE    LIABLE FOR ANY   DIRECT,  INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED   TO, PROCUREMENT OF  SUBSTITUTE GOODS  OR SERVICES; LOSS OF
 * USE, DATA,  OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * You should have received a copy of the  GNU General Public License along
 * with this program; if not, write  to the Free Software Foundation, Inc.,
 * 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifdef __KERNEL__
#ifndef __ASM_SPRUCE_H__
#define __ASM_SPRUCE_H__

#define SPRUCE_PCI_CONFIG_ADDR	0xfec00000
#define SPRUCE_PCI_CONFIG_DATA	0xfec00004

#define SPRUCE_PCI_PHY_IO_BASE	0xf8000000
#define SPRUCE_PCI_IO_BASE	SPRUCE_PCI_PHY_IO_BASE

#define SPRUCE_PCI_SYS_MEM_BASE	0x00000000

#define SPRUCE_PCI_LOWER_MEM	0x80000000
#define SPRUCE_PCI_UPPER_MEM	0x9fffffff
#define SPRUCE_PCI_LOWER_IO	0x00000000
#define SPRUCE_PCI_UPPER_IO	0x03ffffff

#define	SPRUCE_ISA_IO_BASE	SPRUCE_PCI_IO_BASE

#define SPRUCE_MEM_SIZE		0x04000000
#define SPRUCE_BUS_SPEED	66666667

#define SPRUCE_NVRAM_BASE_ADDR	0xff800000
#define SPRUCE_RTC_BASE_ADDR	SPRUCE_NVRAM_BASE_ADDR

#define KEYBOARD_IRQ    22
#define AUX_IRQ 	21

unsigned char spruce_read_keyb_data(void);
unsigned char spruce_read_keyb_status(void);

#define kbd_read_input  spruce_read_keyb_data
#define kbd_read_status spruce_read_keyb_status
#define kbd_write_output(val) *((unsigned char *)0xff810000) = (char)val
#define kbd_write_command(val) *((unsigned char *)0xff810001) = (char)val

/*
 * Serial port defines
 */ 
#define SPRUCE_FPGA_REG_A	0xff820000
#define SPRUCE_UARTCLK_33M	0x02
#define SPRUCE_UARTCLK_IS_33M(reg)	(reg & SPRUCE_UARTCLK_33M)

#define UART0_IO_BASE	0xff600300
#define UART1_IO_BASE	0xff600400

#define RS_TABLE_SIZE	2

#define SPRUCE_BAUD_33M	33000000/64
#define SPRUCE_BAUD_30M	30000000/64
#define BASE_BAUD	SPRUCE_BAUD_33M

#define UART0_INT	3
#define UART1_INT	4

#define STD_UART_OP(num)					\
	{ 0, BASE_BAUD, 0, UART##num##_INT,			\
		(ASYNC_BOOT_AUTOCONF | ASYNC_SKIP_TEST),	\
		iomem_base: UART##num##_IO_BASE,		\
		io_type: SERIAL_IO_MEM},

#define SERIAL_PORT_DFNS	\
	STD_UART_OP(0)		\
	STD_UART_OP(1)

#endif /* __ASM_SPRUCE_H__ */
#endif /* __KERNEL__ */
