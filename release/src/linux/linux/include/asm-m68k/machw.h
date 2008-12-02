/*
** linux/machw.h -- This header defines some macros and pointers for
**                    the various Macintosh custom hardware registers.
**
** Copyright 1997 by Michael Schmitz
**
** This file is subject to the terms and conditions of the GNU General Public
** License.  See the file COPYING in the main directory of this archive
** for more details.
**
*/

#ifndef _ASM_MACHW_H_
#define _ASM_MACHW_H_

/*
 * head.S maps the videomem to VIDEOMEMBASE
 */

#define VIDEOMEMBASE	0xf0000000
#define VIDEOMEMSIZE	(4096*1024)
#define VIDEOMEMMASK	(-4096*1024)

#ifndef __ASSEMBLY__

#include <linux/types.h>


/* hardware stuff */

#define MACHW_DECLARE(name)	unsigned name : 1
#define MACHW_SET(name)		(mac_hw_present.name = 1)
#define MACHW_PRESENT(name)	(mac_hw_present.name)

struct mac_hw_present {
  /* video hardware */
  /* sound hardware */
  /* disk storage interfaces */
  MACHW_DECLARE(MAC_SCSI_80);     /* Directly mapped NCR5380 */
  MACHW_DECLARE(MAC_SCSI_96);     /* 53c9[46] */
  MACHW_DECLARE(MAC_SCSI_96_2);   /* 2nd 53c9[46] Q900 and Q950 */
  MACHW_DECLARE(IDE);             /* IDE Interface */
  /* other I/O hardware */
  MACHW_DECLARE(SCC);             /* Serial Communications Contr. */
  /* DMA */
  MACHW_DECLARE(SCSI_DMA);        /* DMA for the NCR5380 */
  /* real time clocks */
  MACHW_DECLARE(RTC_CLK);         /* clock chip */
  /* supporting hardware */
  MACHW_DECLARE(VIA1);            /* Versatile Interface Ad. 1 */
  MACHW_DECLARE(VIA2);            /* Versatile Interface Ad. 2 */
  MACHW_DECLARE(RBV);             /* Versatile Interface Ad. 2+ */
  /* NUBUS */
  MACHW_DECLARE(NUBUS);           /* NUBUS */
};

extern struct mac_hw_present mac_hw_present;

#endif /* __ASSEMBLY__ */

#endif /* linux/machw.h */
